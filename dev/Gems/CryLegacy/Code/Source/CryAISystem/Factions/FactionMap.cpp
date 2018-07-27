/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "FactionMap.h"


CFactionMap::CFactionMap()
{
	Clear();
	LoadConfig("Scripts/AI/Factions.xml");
}

uint32 CFactionMap::GetFactionCount() const
{
	return m_names.size();
}

const char* CFactionMap::GetFactionName(uint8 faction) const
{
	FactionNames::const_iterator it = m_names.find(faction);
	if (it != m_names.end())
		return it->second.c_str();

	return 0;
}

uint8 CFactionMap::GetFactionID(const char* name) const
{
	FactionIds::const_iterator it = m_ids.find(CONST_TEMP_STRING(name));
	if (it != m_ids.end())
		return it->second;

	return InvalidFactionID;
}

void CFactionMap::Clear()
{
	m_names.clear();
	m_ids.clear();

	for (uint32 i = 0; i < MaxFactionCount; ++i)
		for (uint32 j = 0; j < MaxFactionCount; ++j)
			m_reactions[i][j] = (i != j) ? Hostile : Friendly;
}

void CFactionMap::Reload()
{
	Clear();

	FileNames::iterator it = m_fileNames.begin();
	FileNames::iterator end = m_fileNames.end();

	for ( ; it != end; ++it)
		LoadConfig(it->c_str());
}

struct ReactionInfo
{
	string factionName;
	uint32 line;
	IFactionMap::ReactionType reactionType;
};

bool CFactionMap::LoadConfig(const char* fileName)
{
	if (!gEnv->pCryPak->IsFileExist(fileName))
		return false;

	stl::push_back_unique(m_fileNames, fileName);

	XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(fileName);
	
	if (!rootNode)
	{
		AIWarning("Failed to open XML file '%s'...", fileName);

		return false;
	}

	ReactionType defaultReactions[MaxFactionCount];
	for (uint32 i = 0; i < MaxFactionCount; ++i)
		defaultReactions[i] = Hostile;

	typedef std::vector<ReactionInfo> ReactionInfos;
	typedef std::map<uint8, ReactionInfos> Reactions;
	Reactions reactions;

	const char* tagName = rootNode->getTag();

	if (!_stricmp(tagName, "Factions"))
	{
		int factionNodeCount = rootNode->getChildCount();

		for (int i = 0; i < factionNodeCount; ++i)
		{
			XmlNodeRef factionNode = rootNode->getChild(i);

			if (!_stricmp(factionNode->getTag(), "Faction"))
			{
				const char* name;
				if (!factionNode->getAttr("name", &name))
				{
					AIWarning("Missing 'name' attribute for 'Faction' tag in file '%s' at line %d...", fileName, 
						factionNode->getLine());

					return false;
				}

				uint8 factionID = m_names.size();
				if (factionID >= MaxFactionCount)
				{
					AIWarning("Maximum number of allowed factions reached in file '%s' at line '%d'!", fileName,
						factionNode->getLine());

					return false;
				}

				m_names.insert(FactionNames::value_type(factionID, name));
				AZStd::pair<FactionIds::iterator, bool> iresult = m_ids.insert(FactionIds::value_type(name, factionID));

				if (!iresult.second)
				{
					AIWarning("Duplicate faction '%s' in file '%s' at line %d...", name, fileName, factionNode->getLine());
				
					return false;
				}

				ReactionType defaultReactionType = Hostile;

				const char* defaultReaction;
				if (factionNode->getAttr("default", &defaultReaction) && *defaultReaction)
				{
					if (!GetReactionType(defaultReaction, &defaultReactionType))
					{
						AIWarning("Invalid default reaction '%s' in file '%s' at line '%d'...", 
							defaultReaction, fileName, factionNode->getLine());

						return false;
					}
				}

				defaultReactions[factionID] = defaultReactionType;

				uint32 reactionNodeCount = factionNode->getChildCount();
				for (uint32 j = 0; j < reactionNodeCount; ++j)
				{
					XmlNodeRef reactionNode = factionNode->getChild(j);

					if (!_stricmp(reactionNode->getTag(), "Reaction"))
					{
						const char* faction;
						if (!reactionNode->getAttr("faction", &faction))
						{
							AIWarning("Missing 'faction' attribute for 'Reaction' tag in file '%s' at line %d...", fileName, 
								reactionNode->getLine());

							return false;
						}

						const char* reaction;
						if (!reactionNode->getAttr("reaction", &reaction))
						{
							AIWarning("Missing 'reaction' attribute for 'Reaction' tag in file '%s' at line %d...", fileName,
								reactionNode->getLine());

							return false;
						}

						ReactionType reactionType = Neutral;

						if (!GetReactionType(reaction, &reactionType))
						{
							AIWarning("Invalid reaction '%s' in file '%s' at line '%d'...", reaction, fileName, reactionNode->getLine());

							//return false;
						}

						ReactionInfo info;
						info.factionName = faction;
						info.line = reactionNode->getLine();
						info.reactionType = reactionType;

						std::pair<Reactions::iterator, bool> riresult = 
							reactions.insert(Reactions::value_type(factionID, ReactionInfos()));

						ReactionInfos& infos = riresult.first->second;
						infos.push_back(info);
					}
					else
					{
						AIWarning("Unexpected tag '%s' in file '%s' at line %d...", reactionNode->getTag(), fileName,
							reactionNode->getLine());

						return false;
					}
				}
			}
			else
			{
				AIWarning("Unexpected tag '%s' in file '%s' at line %d...", factionNode->getTag(), fileName, 
					factionNode->getLine());

				return false;
			}
		}
	}
	else
	{
		AIWarning("Unexpected tag '%s' in file '%s' at line %d...", tagName, fileName, rootNode->getLine());

		return false;
	}

	for (uint32 i = 0; i < MaxFactionCount; ++i)
		for (uint32 j = 0; j < MaxFactionCount; ++j)
			m_reactions[i][j] = (i != j) ? defaultReactions[i] : Friendly;

	Reactions::iterator it = reactions.begin();
	Reactions::iterator end = reactions.end();

	for ( ; it != end; ++it)
	{
		ReactionInfos& infos = it->second;

		ReactionInfos::iterator rifit = infos.begin();
		ReactionInfos::iterator rifend = infos.end();

		for ( ; rifit != rifend; ++rifit)
		{
			ReactionInfo& info = *rifit;

			FactionIds::iterator idIt = m_ids.find(info.factionName.c_str());
			if (idIt != m_ids.end())
				SetReaction(it->first, idIt->second, info.reactionType);
			else
			{
				AIWarning("Unknown faction '%s' in file '%s' at line '%d'...", info.factionName.c_str(), fileName, info.line);

				return false;
			}
		}
	}

	return true;
}

void CFactionMap::SetReaction(uint8 factionOne, uint8 factionTwo, IFactionMap::ReactionType reaction)
{
	if ((factionOne < MaxFactionCount) && (factionTwo < MaxFactionCount))
	{
		m_reactions[factionOne][factionTwo] = reaction;

		// Márcio: HAX
		AIObjects::iterator it = GetAISystem()->m_mapFaction.find(factionOne);
		AIObjects::iterator end = GetAISystem()->m_mapFaction.end();
		
		for ( ; it != end; ++it)
		{
			if (it->first == factionOne)
			{
				if (CAIObject* object = it->second.GetAIObject())
				{
					if (CAIActor* actor = object->CastToCAIActor())
						actor->ReactionChanged(factionTwo, reaction);
				}
			}
			else
				break;
		}
	}
}

IFactionMap::ReactionType CFactionMap::GetReaction(uint8 factionOne, uint8 factionTwo) const
{
	if ((factionOne < MaxFactionCount) && (factionTwo < MaxFactionCount))
		return (IFactionMap::ReactionType)m_reactions[factionOne][factionTwo];

	IF_UNLIKELY (factionOne == InvalidFactionID || factionTwo == InvalidFactionID)
		return Neutral;

	return Hostile;
}

void CFactionMap::Serialize(TSerialize ser)
{
	ser.BeginGroup("FactionMap");

	// find highest faction id
	uint32 highestId = 0;

	if (ser.IsWriting())
	{
		FactionIds::iterator it = m_ids.begin();
		FactionIds::iterator end = m_ids.end();

		for ( ; it != end; ++it)
		{
			if (it->second > highestId)
				highestId = it->second;
		}
	}

	ser.Value("SerializedFactionCount", highestId);

	stack_string nameFormatter;
	for (size_t i = 0; i < highestId; ++i)
	{
		for (size_t j = 0; j < highestId; ++j)
		{
			nameFormatter.Format("Reaction_%d_to_%d", i, j);
			ser.Value(nameFormatter.c_str(), m_reactions[i][j]);
		}
	}

	ser.EndGroup();
}

bool CFactionMap::GetReactionType(const char* reactionName, ReactionType* reactionType) const
{
	if (!_stricmp(reactionName, "Friendly"))
		*reactionType = Friendly;
	else if (!_stricmp(reactionName, "Hostile"))
		*reactionType = Hostile;
	else if (!_stricmp(reactionName, "Neutral"))
		*reactionType = Neutral;
	else
		return false;

	return true;
}
