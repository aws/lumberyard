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

#include "SystemComponentFixture.h"
#include "TestAssetCode/MotionEvent.h"
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TwoStringEventData.h>

#include <AzCore/std/algorithm.h>

namespace EMotionFX
{
    struct ExtractEventsParams
    {
        void (*eventFactory)(MotionEventTrack* track);
        float startTime;
        float endTime;
        std::vector<EventInfo> expectedEvents;
    };

    void PrintTo(const EMotionFX::EventInfo::EventState& state, ::std::ostream* os)
    {
        if (state == EMotionFX::EventInfo::EventState::START)
        {
            *os << "EMotionFX::EventInfo::EventState::START";
        }
        else if (state == EMotionFX::EventInfo::EventState::ACTIVE)
        {
            *os << "EMotionFX::EventInfo::EventState::ACTIVE";
        }
        else if (state == EMotionFX::EventInfo::EventState::END)
        {
            *os << "EMotionFX::EventInfo::EventState::END";
        }
    }

    void PrintTo(const EMotionFX::EventInfo& event, ::std::ostream* os)
    {
        *os << "Time: " << event.mTimeValue
            << " State: "
            ;
        PrintTo(event.m_eventState, os);
    }

    void PrintTo(const ExtractEventsParams& object, ::std::ostream* os)
    {
        if (object.eventFactory == &MakeNoEvents)
        {
            *os << "Events: 0";
        }
        else if (object.eventFactory == &MakeOneEvent)
        {
            *os << "Events: 1";
        }
        else if (object.eventFactory == &MakeTwoEvents)
        {
            *os << "Events: 2";
        }
        else if (object.eventFactory == &MakeThreeEvents)
        {
            *os << "Events: 3";
        }
        else if (object.eventFactory == &MakeThreeRangedEvents)
        {
            *os << "Events: 3 (ranged)";
        }
        else
        {
            *os << "Events: Unknown";
        }
        *os << " Start time: " << object.startTime
            << " End time: " << object.endTime
            << " Expected events: ["
            ;
        for (const auto& entry : object.expectedEvents)
        {
            PrintTo(entry, os);
            if (&entry != &(*(object.expectedEvents.end() - 1)))
            {
                *os << ", ";
            }
        }
        *os << ']';
    }

    // This fixture is used for both MotionEventTrack::ProcessEvents and
    // MotionEventTrack::ExtractEvents. Both calls should have similar results,
    // with the exception that ProcessEvents filters out events whose state is
    // ACTIVE.
    class TestExtractProcessEventsFixture
        : public SystemComponentFixture
        , public ::testing::WithParamInterface<ExtractEventsParams>
    {
        // This event handler exists to capture events and put them in a
        // AnimGraphEventBuffer, so that the ExtractEvents and ProcessEvents
        // tests can test the results in a similar fashion
        class TestProcessEventsEventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(TestProcessEventsEventHandler, AZ::SystemAllocator, 0);

            TestProcessEventsEventHandler(AnimGraphEventBuffer* buffer)
                : m_buffer(buffer)
            {
            }

            virtual const AZStd::vector<EventTypes> GetHandledEventTypes() const
            {
                return { EVENT_TYPE_ON_EVENT };
            }

            void OnEvent(const EMotionFX::EventInfo& emfxInfo) override
            {
                m_buffer->AddEvent(emfxInfo);
            }

        private:
            AnimGraphEventBuffer* m_buffer;
        };

    public:
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_motion = SkeletalMotion::Create("TestExtractEventsMotion");
            m_motion->SetMaxTime(2.0f);
            m_motion->GetEventTable()->AutoCreateSyncTrack(m_motion);
            m_track = m_motion->GetEventTable()->GetSyncTrack();

            GetParam().eventFactory(m_track);

            m_actor = Actor::Create("testActor");
            Node* rootNode = Node::Create("rootNode", m_actor->GetSkeleton());
            m_actor->AddNode(rootNode);
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit();

            m_actorInstance = ActorInstance::Create(m_actor);

            m_motionInstance = MotionInstance::Create(m_motion, m_actorInstance, 0);

            m_buffer = new AnimGraphEventBuffer();

            GetEMotionFX().GetEventManager()->AddEventHandler(aznew TestProcessEventsEventHandler(m_buffer));
        }

        void TearDown() override
        {
            delete m_buffer;
            m_motionInstance->Destroy();
            m_motion->Destroy();
            m_actorInstance->Destroy();
            m_actor->Destroy();
            SystemComponentFixture::TearDown();
        }

        void TestEvents(AZStd::function<void(float, float, MotionInstance*)> func)
        {
            const ExtractEventsParams& params = GetParam();

            // Call the function being tested
            func(params.startTime, params.endTime, m_motionInstance);

            // ProcessEvents filters out the ACTIVE events, remove those from our expected results
            AZStd::vector<EventInfo> expectedEvents;
            for (const EventInfo& event : params.expectedEvents)
            {
                if (event.m_eventState != EventInfo::ACTIVE || m_shouldContainActiveEvents)
                {
                    expectedEvents.emplace_back(event);
                }
            }

            EXPECT_EQ(m_buffer->GetNumEvents(), expectedEvents.size()) << "Number of events is incorrect";
            for (uint32 i = 0; i < AZStd::min(m_buffer->GetNumEvents(), static_cast<uint32>(expectedEvents.size())); ++i)
            {
                const EventInfo& gotEvent = m_buffer->GetEvent(i);
                const EventInfo& expectedEvent = expectedEvents[i];
                EXPECT_EQ(gotEvent.mTimeValue, expectedEvent.mTimeValue);
                EXPECT_EQ(gotEvent.m_eventState, expectedEvent.m_eventState);
            }
        }

    protected:
        AnimGraphEventBuffer* m_buffer;
        SkeletalMotion* m_motion;
        MotionInstance* m_motionInstance;
        MotionEventTrack* m_track;
        Actor* m_actor;
        ActorInstance* m_actorInstance;

        // ProcessEvents filters out ACTIVE events. For the ProcessEvents
        // tests, this will be set to false.
        bool m_shouldContainActiveEvents;
    };

    TEST_P(TestExtractProcessEventsFixture, TestExtractEvents)
    {
        m_shouldContainActiveEvents = true;
        TestEvents([this](float startTime, float endTime, MotionInstance* motionInstance)
        {
            return this->m_track->ExtractEvents(startTime, endTime, motionInstance, m_buffer);
        });
    }

    TEST_P(TestExtractProcessEventsFixture, TestProcessEvents)
    {
        m_shouldContainActiveEvents = false;
        TestEvents([this](float startTime, float endTime, MotionInstance* motionInstance)
        {
            return this->m_track->ProcessEvents(startTime, endTime, motionInstance);
        });
    }

    std::vector<ExtractEventsParams> extractEventTestData {
        {
            MakeThreeEvents,
            0.0f,
            1.0f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                }
            }
        },
        {
            MakeThreeEvents,
            0.0f,
            1.5f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    1.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                }
            }
        },
        {
            // Processing from before a ranged event begins to the middle of
            // that event should give a start event and an active event
            MakeThreeRangedEvents,
            0.0f,
            0.3f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.3f, // This will be equal to the end time parameter passed to the method
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::ACTIVE
                }
            }
        },
        {
            // Processing from before a ranged event begins to after the end of
            // that event should give a start event and an end event
            MakeThreeRangedEvents,
            0.0f,
            0.6f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                }
            }
        },
        {
            // Processing from the middle of a ranged event to after the end of
            // that event should give just an end event
            MakeThreeRangedEvents,
            0.3f,
            0.6f,
            std::vector<EventInfo> {
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                }
            }
        },
        {
            // Each ranged event processed whose start time is traversed
            // generates 2 event infos
            MakeThreeRangedEvents,
            0.0f,
            0.9f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                },
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.9f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::ACTIVE
                }
            }
        },

        // Now the backwards playback cases
        {
            MakeThreeEvents,
            1.0f,
            0.0f,
            std::vector<EventInfo> {
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                }
            }
        },
        {
            MakeThreeEvents,
            1.5f,
            0.0f,
            std::vector<EventInfo> {
                EventInfo {
                    1.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                }
            }
        },
        {
            // Processing from the middle of a ranged event to before that
            // event begins should give an end event
            MakeThreeRangedEvents,
            0.3f,
            0.0f,
            std::vector<EventInfo> {
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                }
            }
        },
        {
            // Processing from after a ranged event ends to before the
            // beginning of that event should give a start event and an end
            // event
            MakeThreeRangedEvents,
            0.6f,
            0.0f,
            std::vector<EventInfo> {
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                }
            }
        },
        {
            // Processing from after the end of an event to the middle of a
            // ranged event should give a start event and an active event
            MakeThreeRangedEvents,
            0.6f,
            0.3f,
            std::vector<EventInfo> {
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.3f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::ACTIVE
                }
            }
        },
        {
            // Start in the middle of a ranged event while playing backwards
            MakeThreeRangedEvents,
            0.9f,
            0.0f,
            std::vector<EventInfo> {
                EventInfo {
                    0.75f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                },
                EventInfo {
                    0.5f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::START
                },
                EventInfo {
                    0.25f,
                    nullptr, nullptr, nullptr,
                    EMotionFX::EventInfo::EventState::END
                }
            }
        }
    };

    INSTANTIATE_TEST_CASE_P(TestExtractProcessEvents, TestExtractProcessEventsFixture,
        ::testing::ValuesIn(extractEventTestData));
} // end namespace EMotionFX
