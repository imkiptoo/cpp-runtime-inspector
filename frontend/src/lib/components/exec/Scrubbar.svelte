<script lang="ts">
	import { onMount } from 'svelte';

	interface Props {
		value: number;
		max: number;
		totalSteps: number;
		onChange: (value: number) => void;
		disabled?: boolean;
	}

	let { value, max, totalSteps, onChange, disabled = false }: Props = $props();

	let trackRef: HTMLDivElement | undefined = $state();
	let isDragging = $state(false);

	const percentage = $derived(max > 0 ? (value / max) * 100 : 0);

	// Major tick every N steps based on total
	const majorEvery = $derived(totalSteps > 15 ? 5 : 2);

	function updateValue(clientX: number) {
		if (!trackRef || disabled || max <= 0) return;
		const rect = trackRef.getBoundingClientRect();
		const x = clientX - rect.left;
		const pct = Math.max(0, Math.min(1, x / rect.width));
		const newValue = Math.round(pct * max);
		onChange(newValue);
	}

	function handlePointerDown(event: PointerEvent) {
		if (disabled) return;
		isDragging = true;
		updateValue(event.clientX);
	}

	function handleKeyDown(event: KeyboardEvent) {
		if (disabled) return;
		if (event.key === 'ArrowLeft') {
			event.preventDefault();
			onChange(Math.max(0, value - 1));
		} else if (event.key === 'ArrowRight') {
			event.preventDefault();
			onChange(Math.min(max, value + 1));
		} else if (event.key === 'Home') {
			event.preventDefault();
			onChange(0);
		} else if (event.key === 'End') {
			event.preventDefault();
			onChange(max);
		}
	}

	// Handle dragging with pointer events
	$effect(() => {
		if (!isDragging) return;

		function handlePointerMove(e: PointerEvent) {
			updateValue(e.clientX);
		}

		function handlePointerUp() {
			isDragging = false;
		}

		window.addEventListener('pointermove', handlePointerMove);
		window.addEventListener('pointerup', handlePointerUp);

		return () => {
			window.removeEventListener('pointermove', handlePointerMove);
			window.removeEventListener('pointerup', handlePointerUp);
		};
	});
</script>

<!-- svelte-ignore a11y_no_static_element_interactions -->
<div
	bind:this={trackRef}
	class="relative h-7 flex-1 cursor-pointer select-none rounded-sm focus:outline focus:outline-1 focus:outline-brand-400 {disabled ? 'pointer-events-none opacity-40' : ''}"
	onpointerdown={handlePointerDown}
	onkeydown={handleKeyDown}
	role="slider"
	aria-label="step"
	aria-valuemin={1}
	aria-valuemax={totalSteps}
	aria-valuenow={value + 1}
	tabindex={disabled ? -1 : 0}
	data-testid="exec-scrub"
	data-dragging={isDragging || undefined}
>
	<!-- Track -->
	<div class="absolute left-0 right-0 top-1/2 h-1.5 -translate-y-1/2 overflow-hidden rounded-sm border border-line bg-islands-100 dark:bg-islands-800">
		<!-- Fill -->
		<div
			class="h-full bg-brand-500 transition-[width] duration-75 ease-linear"
			style="width: {percentage}%"
		></div>
	</div>

	<!-- Tick marks -->
	{#if totalSteps > 1 && totalSteps <= 200}
		<div class="pointer-events-none absolute left-0 right-0 top-1/2 h-3 -translate-y-1/2">
			{#each Array(totalSteps) as _, i}
				{@const isMajor = i % majorEvery === 0}
				<span
					aria-hidden
					class="absolute w-[1px] {isMajor ? 'bg-islands-400 dark:bg-islands-500' : 'bg-islands-300 dark:bg-islands-600 opacity-70'}"
					style="
						left: {(i / (totalSteps - 1)) * 100}%;
						height: {isMajor ? '16px' : '10px'};
						top: {isMajor ? '-3px' : '0'};
					"
				></span>
			{/each}
		</div>
	{/if}

	<!-- Thumb -->
	<div
		class="absolute top-1/2 h-[18px] w-3 -translate-x-1/2 -translate-y-1/2 rounded-sm border border-brand-400 bg-brand-500 shadow-sm transition-transform {isDragging ? 'scale-110' : 'hover:scale-105'}"
		style="left: {percentage}%"
	>
		<!-- Center line on thumb -->
		<span
			aria-hidden
			class="absolute left-1/2 top-0.5 bottom-0.5 w-[1px] -translate-x-1/2 bg-white/30"
		></span>
	</div>
</div>
