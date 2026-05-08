<script lang="ts">
	import { onMount } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import { getThemeLabel, type ThemePreference } from '$lib/theme/theme';
	import type { RoutingMode } from '$lib/viz/routeEdges';
	import type { HeapDensity } from '$lib/viz/layoutHeap';

	interface Props {
		onClose: () => void;
	}

	let { onClose }: Props = $props();

	let menuRef: HTMLDivElement | undefined = $state();

	const themeOptions: ThemePreference[] = ['system', 'light', 'dark'];
	const routingOptions: { value: RoutingMode; label: string }[] = [
		{ value: 'curved', label: 'Curved' },
		{ value: 'straight', label: 'Straight' },
		{ value: 'orthogonal', label: 'Orthogonal' }
	];
	const densityOptions: { value: HeapDensity; label: string }[] = [
		{ value: 'compact', label: 'Compact' },
		{ value: 'normal', label: 'Normal' },
		{ value: 'spread', label: 'Spread' }
	];

	function handleClickOutside(event: MouseEvent) {
		if (menuRef && !menuRef.contains(event.target as Node)) {
			onClose();
		}
	}

	onMount(() => {
		document.addEventListener('click', handleClickOutside, true);
		return () => document.removeEventListener('click', handleClickOutside, true);
	});
</script>

<div
	bind:this={menuRef}
	class="absolute right-0 top-full mt-2 w-72 bg-white dark:bg-islands-800 border border-line rounded-md shadow-islands-lg z-50"
>
	<div class="p-3 space-y-3">
		<!-- Theme -->
		<div>
			<label class="block text-sm font-medium text-faded mb-2">Theme</label>
			<div class="btn-bar w-full">
				{#each themeOptions as option}
					<button
						class="btn-chonky btn-chonky-sm h-7 flex-1 {appState.themePreference === option ? 'btn-chonky-active' : 'btn-chonky-secondary'}"
						onclick={() => appState.setThemePreference(option)}
					>
						<span>{getThemeLabel(option)}</span>
					</button>
				{/each}
			</div>
		</div>

		<!-- Pointer Routing -->
		<div>
			<label class="block text-sm font-medium text-faded mb-2">Pointer Arrows</label>
			<div class="btn-bar w-full">
				{#each routingOptions as option}
					<button
						class="btn-chonky btn-chonky-sm h-7 flex-1 {appState.pointerRouting === option.value ? 'btn-chonky-active' : 'btn-chonky-secondary'}"
						onclick={() => appState.setPointerRouting(option.value)}
					>
						<span>{option.label}</span>
					</button>
				{/each}
			</div>
		</div>

		<!-- Heap Density -->
		<div>
			<label class="block text-sm font-medium text-faded mb-2">Heap Spacing</label>
			<div class="btn-bar w-full">
				{#each densityOptions as option}
					<button
						class="btn-chonky btn-chonky-sm h-7 flex-1 {appState.heapDensity === option.value ? 'btn-chonky-active' : 'btn-chonky-secondary'}"
						onclick={() => appState.setHeapDensity(option.value)}
					>
						<span>{option.label}</span>
					</button>
				{/each}
			</div>
		</div>

		<!-- Auto-play on Run -->
		<div class="flex items-center justify-between">
			<label class="text-sm font-medium text-faded">Auto-play on Run</label>
			<button
				type="button"
				role="switch"
				aria-checked={appState.autoPlayOnRun}
				onclick={() => appState.setAutoPlayOnRun(!appState.autoPlayOnRun)}
				class="relative inline-flex h-5 w-9 items-center rounded-full transition-colors {appState.autoPlayOnRun ? 'bg-brand-500' : 'bg-islands-300 dark:bg-islands-600'}"
			>
				<span
					class="inline-block h-4 w-4 transform rounded-full bg-white shadow-sm transition-transform {appState.autoPlayOnRun ? 'translate-x-4' : 'translate-x-0.5'}"
				></span>
			</button>
		</div>

		<!-- Play Speed -->
		<div>
			<div class="flex items-center justify-between mb-2">
				<label class="text-sm font-medium text-faded">Play Speed</label>
				<span class="text-sm text-islands-600 dark:text-islands-300 font-mono">
					{appState.playSpeed < 1000 ? `${appState.playSpeed}ms` : `${(appState.playSpeed / 1000).toFixed(1)}s`}
				</span>
			</div>
			<div class="flex items-center gap-2">
				<span class="text-sm text-faded">Fast</span>
				<input
					type="range"
					min="100"
					max="2000"
					step="100"
					value={appState.playSpeed}
					oninput={(e) => appState.setPlaySpeed(parseInt(e.currentTarget.value))}
					class="flex-1 h-1.5 bg-islands-200 dark:bg-islands-700 rounded-full appearance-none cursor-pointer accent-brand-500"
				/>
				<span class="text-sm text-faded">Slow</span>
			</div>
		</div>

	</div>

	<!-- Footer -->
	<div class="px-3 py-3 border-t border-line text-sm text-faded">
		C++ Runtime Inspector v1.0.0
	</div>
</div>
