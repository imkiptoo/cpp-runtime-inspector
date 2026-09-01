<script lang="ts">
	import { tick } from 'svelte';
	import type { StackFrame, EncodedValue } from '$lib/trace/schema';
	import { appState } from '$lib/state/app.svelte';
	import { capturePositions, calculateTransitions, animateFlip, animateEnter } from '$lib/anim/flip';
	import FrameCard from './FrameCard.svelte';
	import VarTable from './VarTable.svelte';

	interface Props {
		frames: StackFrame[];
		globals: Record<string, EncodedValue>;
		orderedGlobals: string[];
	}

	let { frames, globals, orderedGlobals }: Props = $props();

	let stackContainerRef: HTMLDivElement | undefined = $state();
	let previousPositions = $state<Map<string, { id: string; rect: { x: number; y: number; width: number; height: number }; opacity: number }>>(new Map());
	let previousStepIndex = $state(-1);

	const hasGlobals = $derived(orderedGlobals.length > 0);

	// Reverse frames so most recent is at top
	const displayFrames = $derived([...frames].reverse());

	// Capture positions before step changes
	$effect.pre(() => {
		const stepIndex = appState.stepIndex;
		if (stackContainerRef && stepIndex !== previousStepIndex) {
			previousPositions = capturePositions(stackContainerRef);
		}
	});

	// Animate after step changes
	$effect(() => {
		const stepIndex = appState.stepIndex;
		const currentFrames = displayFrames;

		if (!stackContainerRef || stepIndex === previousStepIndex) {
			if (stepIndex !== previousStepIndex) {
				previousStepIndex = stepIndex;
			}
			return;
		}

		tick().then(() => {
			if (!stackContainerRef) return;

			const currentPositions = capturePositions(stackContainerRef);
			const { moved, entered } = calculateTransitions(previousPositions, currentPositions);

			// Animate moved elements
			for (const transition of moved) {
				const element = stackContainerRef.querySelector(`[data-flip-id="${transition.id}"]`);
				if (element instanceof HTMLElement) {
					animateFlip(element, transition, 200);
				}
			}

			// Animate new elements (new stack frames)
			for (const id of entered) {
				const element = stackContainerRef.querySelector(`[data-flip-id="${id}"]`);
				if (element instanceof HTMLElement) {
					animateEnter(element, 150);
				}
			}

			previousStepIndex = stepIndex;
			previousPositions = currentPositions;
		});
	});
</script>

<div class="h-full overflow-auto">
	<!-- Stack frames -->
	<div bind:this={stackContainerRef} class="flex flex-col gap-1.5 px-3 pb-5" data-testid="stack-frames">
		{#if displayFrames.length === 0}
			<p class="px-3 py-2 font-mono text-[11px] text-islands-400 dark:text-islands-500">
				Run to see stack frames.
			</p>
		{:else}
			{#each displayFrames as frame, index (frame.frame_id)}
				<div data-flip-id="frame-{frame.frame_id}">
					<FrameCard
						{frame}
						isTop={index === 0}
					/>
				</div>
			{/each}
		{/if}
	</div>

	<!-- Globals -->
	{#if hasGlobals}
		<div class="border-t border-line px-3 py-3">
			<div class="font-mono text-[10px] uppercase tracking-widest text-islands-400 dark:text-islands-500 mb-2">
				Globals
			</div>
			<div class="overflow-hidden rounded-sm border border-line bg-white dark:bg-islands-800 px-2.5 pb-2 pt-1">
				<!-- Globals have no runtime address, so that column is omitted. -->
				<VarTable
					names={orderedGlobals}
					values={globals}
					sizes={appState.currentStep?.global_sizes}
					types={appState.currentStep?.global_types}
				/>
			</div>
		</div>
	{/if}
</div>
