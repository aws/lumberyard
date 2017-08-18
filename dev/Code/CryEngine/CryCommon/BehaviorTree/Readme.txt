
=================
Behavior Tree
=================

> Examples

------------------------------------------------------
Example - Perform one action (smallest tree possible)
------------------------------------------------------
<BehaviorTree>
	<Root>
		<Log message="This is a log message."/>
	</Root>
</BehaviorTree>



------------------------------------------------------
Example - Perform actions in a sequence
------------------------------------------------------
<BehaviorTree>
	<Root>
		<Sequence>
			<Log message="This is a log message."/>
			<Wait duration="1.0"/>
			<Log message="This is another log message."/>
			<Wait duration="2.0"/>
		</Sequence>
	</Root>
</BehaviorTree>



------------------------------------------------------
Example - Loop Decorator
------------------------------------------------------
<BehaviorTree>
	<Root>
		<Loop>
			<Sequence>
				<Log message="This is a log message."/>
				<Wait duration="2.0"/>
			</Sequence>
		</Loop>
	</Root>
</BehaviorTree>