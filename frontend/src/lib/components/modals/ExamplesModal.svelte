<script lang="ts">
	import Modal from './Modal.svelte';
	import { examples, type Example } from '$lib/examples/examples';
	import { shortcuts } from '$lib/platform/kbd';

	interface Props {
		open: boolean;
		onClose: () => void;
		onSelect: (code: string) => void;
	}

	let { open, onClose, onSelect }: Props = $props();

	function handleSelect(example: Example) {
		onSelect(example.code);
	}

	function getComplexityColors(complexity: 1 | 2 | 3): string[] {
		// Returns colors for each dot based on complexity level
		// complexity 1: first dot lit, rest dim
		// complexity 2: first two dots lit, last dim
		// complexity 3: all three dots lit
		const litColor = 'bg-amber-500';
		const dimColor = 'bg-islands-400 dark:bg-islands-600';

		if (complexity === 1) {
			return [litColor, dimColor, dimColor];
		} else if (complexity === 2) {
			return [litColor, litColor, dimColor];
		} else {
			return [litColor, litColor, litColor];
		}
	}
</script>

<Modal {open} {onClose} title="EXAMPLES">
	{#snippet children()}
		<div class="p-3">
			<!-- Subtitle -->
			<p class="text-sm text-faded pb-3 border-b border-line">
				Browse - load - fork. Picking an example replaces the current editor contents.
			</p>

			<!-- Examples grid -->
			<div class="grid pt-3 grid-cols-1 sm:grid-cols-2 gap-3 max-h-[420px] overflow-auto">
				{#each examples as example (example.id)}
					<button
						class="text-left p-3 bg-islands-50 dark:bg-islands-800/50 hover:bg-islands-100 dark:hover:bg-islands-700/50 border border-line rounded-md transition-colors flex flex-col"
						onclick={() => handleSelect(example)}
					>
						<!-- Category -->
						<span class="text-[11px] uppercase tracking-wider text-faded mb-1">
							{example.category}
						</span>

						<!-- Title -->
						<h3 class="font-semibold text-islands-900 dark:text-islands-100 mb-1">{example.title}</h3>

						<!-- Description -->
						<p class="text-[13px] text-islands-600 dark:text-islands-400 mb-1.5 flex-1">{example.description}</p>

						<!-- Footer: complexity dots + steps -->
						<div class="flex items-center justify-between">
							<!-- Complexity dots -->
							<div class="flex items-center gap-1">
								{#each getComplexityColors(example.complexity) as color}
									<span class="w-2 h-2 rounded-full {color}"></span>
								{/each}
							</div>

							<!-- Step count -->
							<span class="text-[13px] text-islands-500 dark:text-islands-400 font-mono">
								~{example.steps} steps
							</span>
						</div>
					</button>
				{/each}
			</div>

			<!-- Hint -->
			<div class="mt-3 pt-3 border-t border-line text-sm text-faded">
				Tip: <kbd class="px-1.5 py-0.5 bg-islands-100 dark:bg-islands-800 rounded-sm border border-line font-mono">{shortcuts.examples()}</kbd> opens this modal from anywhere.
			</div>
		</div>
	{/snippet}
</Modal>
