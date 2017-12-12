; ------------------------------------------------------------------------------
; ------------------------------------------------------------------------------
; All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
; its licensors.
;
; For complete copyright and license terms please see the LICENSE at the root of this
; distribution(the "License").All use of this software is governed by the License,
; or, if provided, by the license below or the license accompanying this file.Do not
; remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; ------------------------------------------------------------------------------
; ------------------------------------------------------------------------------



;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
; This is AZStd visualizer code for Visual Studio. 
; TODO: Copy and paste this into "Visual Studio Folder\Common7\Packages\Debugger\autoexp.dat"
; Insert this into [Visualizer] section, usually after std!
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; AZStd::vector
;------------------------------------------------------------------------------
AZStd::vector<*>{
	children
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $c.CONTAINER_VERSION == 1 ) 
		(  
		  	#(	capacity: $c.m_end - $c.m_start,
				#array
		    		(
					expr :	($c.m_start)[$i],  
					size :	$c.m_last-$c.m_start
				)
			)
		)
	)
	
	preview
	( 
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("vector[",$e.m_last - $e.m_start ,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::fixed_vector<*,*>{
		; In VS2010 x64 there is a bug with pointer math when casting addresses on the stack.
		; it's really hard to guess that if we are in stack space so we must live with it.
	children
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $c.CONTAINER_VERSION == 1 ) 
		(  
		  	#(	capacity: $T2,
				#array
		    		(
					expr :	(($T1*)(&$c.m_data))[$i],  
					size :	$e.m_last-($T1*)(&$e.m_data)
				)
			)
		)
	)
	
	preview
	( 
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("fixed_vector[",$e.m_last-($T1*)(&$e.m_data),"] capacity ", $T2) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
; AZStd::array
;------------------------------------------------------------------------------
AZStd::array<*,*>{
	children(
		; If the container has changed we need to make sure we update this code too.
		#if( $c.CONTAINER_VERSION == 1 ) 
		( #array(expr: $e.m_elements[$i], size: $T2) )
	)
	preview(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("array[",$T2 ,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
; AZStd::list,AZStd::forward_list,AZStd::fixed_list,AZStd::intrusive_list
;------------------------------------------------------------------------------
AZStd::list<*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(
		#list
		(
			head : $c.m_head.m_next, 
			size : $c.m_numElements, 
			next : m_next
		) :  *($T1*)(&($e) + 1)
		)
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("list[",$e.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}
AZStd::forward_list<*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#list
		(
			head : $c.m_head.m_next, 
			size : $c.m_numElements, 
			next : m_next
		) :  *($T1*)(&($e) + 1)
		) 
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("list[",$e.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::fixed_list<*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#list
		(
			head : $c.m_head.m_next, 
			size : $c.m_numElements, 
			next : m_next
		) :  *($T1*)(&($e) + 1)
		) 
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("fixed_list[",$e.m_numElements,"] capacity ", $T2) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::fixed_forward_list<*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#list
		(
			head : $c.m_head.m_next, 
			size : $c.m_numElements, 
			next : m_next
		) :  *($T1*)(&($e) + 1)
		) 
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("fixed_forward_list[",$e.m_numElements,"] capacity ", $T2) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::intrusive_list<*,AZStd::list_base_hook<*> >{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#list
		(
			; This is such a hack. intrusive_list_node has second template param, so it's not space after template sensitive.
			head : (AZStd::intrusive_list_node<$T1,0>*)(($T1*)$c.m_root.m_buffer)->m_next,
			size : $c.m_numElements, 
			next : m_next
		) : *($T1*)(&$e)
		) 
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("intrusive_list[",$e.m_numElements,"] using base hook") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::intrusive_list<*,AZStd::list_member_hook<*,*> >{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		; At the moment next is a member not expression which we can't offset from the object to the hook node,
		; We can either wait for VS to enable this, write our own DLL or redesign the member hook to point to the 
		; next member hook (which will likely happen soon, since we don't want to store a node as a root). For
		; now we display the root node and the user can manually traverse it's list.
		#( root: *((AZStd::intrusive_list_node<$T1,0>*)($c.m_root.m_buffer + $T3))->m_next )
;		#list
;		(
;			; This is such a hack. intrusive_list_node has second template param, so it's not space after template sensitive.
;			; $T3 is the offset of the Hook in the node structure.
;			head : (AZStd::intrusive_list_node<$T1,0>*)($c.m_root.m_buffer + $T3),
;			size : $c.m_numElements, 
;			next : $T3->m_next
;		) : $e ; *($T1*)( (char*)&$e - $T3)
		) 
	)		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("intrusive_list[",$e.m_numElements,"] using member hook at ", $T3, " offset ") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::intrusive_slist<*,AZStd::slist_base_hook<*> >{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#list
		(
			; This is such a hack. intrusive_list_node has second template param, so it's not space after template sensitive.
			head : (AZStd::intrusive_slist_node<$T1,0>*)(($T1*)$c.m_root.m_buffer)->m_next,
			size : $c.m_numElements, 
			next : m_next
		) : *($T1*)(&$e)
		) 
	)		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("intrusive_slist[",$e.m_numElements,"] using base hook") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::intrusive_slist<*,AZStd::slist_member_hook<*,*> >{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		; At the moment next is a member not expression which we can't offset from the object to the hook node,
		; We can either wait for VS to enable this, write our own DLL or redesign the member hook to point to the 
		; next member hook (which will likely happen soon, since we don't want to store a node as a root). For
		; now we display the root node and the user can manually traverse it's list.
		#( root: *((AZStd::intrusive_slist_node<$T1,0>*)($c.m_root.m_buffer + $T3))->m_next )
;		#list
;		(
;			; This is such a hack. intrusive_list_node has second template param, so it's not space after template sensitive.
;			; $T3 is the offset of the Hook in the node structure.
;			head : (AZStd::intrusive_slist_node<$T1,0>*)($c.m_root.m_buffer + $T3),
;			size : $c.m_numElements, 
;			next : $T3->m_next
;		) : $e ; *($T1*)( (char*)&$e - $T3) 
		) 
	)		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("intrusive_slist[",$e.m_numElements,"] using member hook at ", $T3, " offset ") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::list_const_iterator<*>|AZStd::list_iterator<*>{
	children
	(
		#if( $e.ITERATOR_VERSION == 1 ) (	  	
		#(
			ptr: [ (void*)($c.m_node + 1), x],
			value: *($T1*)($c.m_node + 1)
		)
		)
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.ITERATOR_VERSION == 1 ) 
		( #(*($T1*)($c.m_node + 1)) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::forward_list_const_iterator<*>|AZStd::forward_list_iterator<*>{
	children
	(
		#if( $e.ITERATOR_VERSION == 1 ) (	  	
		#(
			ptr: [ (void*)($c.m_node + 1), x],
			value: *($T1*)($c.m_node + 1)
		)
		)
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.ITERATOR_VERSION == 1 ) 
		( #(*($T1*)($c.m_node + 1)) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

AZStd::intrusive_list<*>::const_iterator_impl|AZStd::intrusive_list<*>::iterator_impl|AZStd::intrusive_slist<*>::const_iterator_impl|AZStd::intrusive_slist<*>::iterator_impl{
	children
	(
		#if( $e.ITERATOR_VERSION == 1 ) (	  	
		#( node: *($e.m_node) )
		)
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.ITERATOR_VERSION == 1 ) 
		( $e.m_node )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::deque
;------------------------------------------------------------------------------
AZStd::deque<*,*,*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) (
		#array
		(
			expr : $c.m_map[ (($i + $c.m_firstOffset) / $T3 ) % $c.m_mapSize][($i + $c.m_firstOffset) % $T3],  
			size : $c.m_size
		)
		)
	)

	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 		
		( #("deque[", $e.m_size,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)		       
}

AZStd::deque<*,*,*,*>::const_iterator_impl|AZStd::deque<*,*,*,*>::iterator_impl{
	children
	(
		#if( $e.ITERATOR_VERSION == 1 ) (
		#if( ($e.m_container->m_firstOffset + $e.m_container->m_size) > $e.m_offset) 
	   	(
			#( value: $e.m_container->m_map[ ($e.m_offset / $T3) % $e.m_container->m_mapSize][$e.m_offset % $T3] )
		)
		#else
		(	#( end : 0 ) )
		)
	)
	preview
	(
		#if( $e.ITERATOR_VERSION == 1 ) (
		#if( $e.m_offset >= $e.m_container->m_firstOffset + $e.m_container->m_size )
		(
	   		#("<end>")
		)
		#else(
		#(
			"deque[", $e.m_offset - $e.m_container->m_firstOffset, "] = ",
			$e.m_container->m_map[ ($e.m_offset / $T3) % $e.m_container->m_mapSize][$e.m_offset % $T3] )
		)
		)
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::queue
;------------------------------------------------------------------------------
AZStd::queue<*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(	
			#( container: $c.m_container ) 
		)
	)

	preview
	( 
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 		
		( #( "queue( ",$e.m_container," )") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::priority_queue
;------------------------------------------------------------------------------
AZStd::priority_queue<*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(	
			#( container: $c.m_container ) 
		)
	)

	preview
	( 
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 		
		( #( "priority_queue( ",$e.m_container," )") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::stack
;------------------------------------------------------------------------------
AZStd::stack<*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(	
			#( container: $c.m_container ) 
		)
	)

	preview
	( 
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 		
		( #( "stack( ",$e.m_container," )") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::ordered containers
;------------------------------------------------------------------------------
AZStd::rbtree_const_iterator<*>|AZStd::rbtree_iterator<*>{
	children
	(
		#if( $e.ITERATOR_VERSION == 1 ) (	  	
		#(
			ptr: [ (void*)($c.m_node), x],
			value: #(*($c.m_node))
		)
		)
	)
		          
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.ITERATOR_VERSION == 1 ) 
		( #(*($c.m_node)) )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}
AZStd::set<*,*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(
			#tree
			(
				head : $c.m_tree.m_head.m_parentColor,
				skip : &($c.m_tree.m_head),
				left : m_left,
				right : m_right,
				size  : $c.m_tree.m_numElements
			) :  *($T1*)(&($e) + 1)
		)
	)
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("set[",$e.m_tree.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}
AZStd::multiset<*,*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(
			#tree
			(
				head : $c.m_tree.m_head.m_parentColor,
				skip : &($c.m_tree.m_head),
				left : m_left,
				right : m_right,
				size  : $c.m_tree.m_numElements
			) :  *($T1*)(&($e) + 1)
		)
	)
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("multiset[",$e.m_tree.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}
AZStd::map<*,*,*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(
			#tree
			(
				head : $c.m_tree.m_head.m_parentColor,
				skip : &($c.m_tree.m_head),
				left : m_left,
				right : m_right,
				size  : $c.m_tree.m_numElements
			) :   *(AZStd::pair<$T1,$T2>*)(&($e) + 1))
		)
	)
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("map[",$e.m_tree.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}
AZStd::multimap<*,*,*,*>{
	children
	(
		#if( $c.CONTAINER_VERSION == 1 ) 
		(
			#tree
			(
				head : $c.m_tree.m_head.m_parentColor,
				skip : &($c.m_tree.m_head),
				left : m_left,
				right : m_right,
				size  : $c.m_tree.m_numElements
			) :  *(AZStd::pair<$T1,$T2>*)(&($e) + 1)
		)
	)
	preview
	(
		; If the container has changed we need to make sure we update this code too.
		#if( $e.CONTAINER_VERSION == 1 ) 
		( #("multimap[",$e.m_tree.m_numElements,"]") )
		#else
		( "Please run AZStd/debugger tools/smartdebug/smartdebug.exe tool. It needs to update the debugger." )
	)
}

;------------------------------------------------------------------------------
;  AZStd::unordered and hash-table
;------------------------------------------------------------------------------
AZStd::hash_table<*>|AZStd::unordered_map<*>|AZStd::unordered_multimap<*>|AZStd::unordered_set<*>|AZStd::unordered_multiset<*>{
	
	children
	(
		#(		
		elements : $c.m_data.m_list, 
		load factor : #if( $c.m_data.m_numBuckets > 0 ) ( [(float)$c.m_data.m_list.m_numElements / (float)$c.m_data.m_numBuckets,g] ) #else ( 0.0g ),
		max load factor: [$c.m_data.m_max_load_factor,g],
		buckets : $c.m_data.m_vector
		)
	)
	preview
	(
		#("hash_map[", $e.m_data.m_list.m_numElements, "]")
	)		
}

AZStd::fixed_unordered_map<*,*,*,*,*,*>|AZStd::fixed_unordered_multimap<*,*,*,*,*,*>{
	
	children
	(
		#(		
		elements : $c.m_data.m_list, 
		buckets : $c.m_data.m_vector
		)
	)
	preview
	(
		#("fixed_hash_map[", $e.m_data.m_list.m_numElements, "] capacity ",$T4, " buckets ",$T3)
	)		
}


;------------------------------------------------------------------------------
; AZStd::pair
;------------------------------------------------------------------------------
AZStd::pair<*>{
    preview
    (
        #(
            "(",$e.first,", ",$e.second,")"
        )
    )
}

;------------------------------------------------------------------------------
;  AZStd::bitset
;------------------------------------------------------------------------------
AZStd::bitset<*>{
	preview (
		#(
			"bitset[",
			$T1,
			"]"
		)
	)

	children (
		#array(
			expr: [($e.m_bits[$i / $e.BitsPerWord] >> ($i % $e.BitsPerWord)) & 1,d],
			size: $T1
		)
	)
}
AZStd::bitset<*>::reference{
	preview (
		[($e.m_bitSet.m_bits[$i / $e.m_bitSet.BitsPerWord] >> ($e.m_pos % $e.m_bitSet.BitsPerWord)) & 1,d]
	)

	children (
		#(
			#([bitset] : $e.m_bitSet),
			#([pos] : $e.m_pos)
		)
	)
}

;------------------------------------------------------------------------------
; AZStd checked iterators
;------------------------------------------------------------------------------
AZStd::Debug::checked_randomaccess_iterator<*>|AZStd::Debug::checked_bidirectional_iterator<*>|AZStd::Debug::checked_input_iterator<*>|AZStd::Debug::checked_forward_iterator<*>|AZStd::Debug::checked_output_iterator<*>{
	preview 
	(
		$e.m_iter
	)
	
	children
	(
		#( checked iterator: $e.m_iter )
	)
}

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
; End of AZStd visualizer code
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------