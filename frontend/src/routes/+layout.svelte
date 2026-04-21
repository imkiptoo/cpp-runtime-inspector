<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import TopBar from '$lib/components/layout/TopBar.svelte';
	import ExamplesModal from '$lib/components/modals/ExamplesModal.svelte';
	import { matchesShortcut } from '$lib/platform/kbd';
	import '../app.css';

	let { children } = $props();

	let examplesModalOpen = $state(false);

	function handleKeydown(event: KeyboardEvent) {
		// Ignore if in input/textarea (but not CodeMirror)
		const target = event.target as HTMLElement;
		if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA') {
			return;
		}

		// Cmd/Ctrl+K - Open examples
		if (matchesShortcut(event, ['mod', 'k'])) {
			event.preventDefault();
			examplesModalOpen = true;
			return;
		}

		// Cmd/Ctrl+Enter - Run
		if (matchesShortcut(event, ['mod', 'enter'])) {
			event.preventDefault();
			appState.run();
			return;
		}

		// Escape - Close modal
		if (event.key === 'Escape' && examplesModalOpen) {
			event.preventDefault();
			examplesModalOpen = false;
			return;
		}

		// Navigation shortcuts (only when not in editor)
		if (!target.closest('.cm-editor')) {
			// Arrow keys for stepping
			if (event.key === 'ArrowRight' && !event.metaKey && !event.ctrlKey) {
				event.preventDefault();
				appState.stepForward();
				return;
			}

			if (event.key === 'ArrowLeft' && !event.metaKey && !event.ctrlKey) {
				event.preventDefault();
				appState.stepBackward();
				return;
			}

			// Cmd/Ctrl+Arrow for start/end
			if (matchesShortcut(event, ['mod', 'arrowleft'])) {
				event.preventDefault();
				appState.goToStart();
				return;
			}

			if (matchesShortcut(event, ['mod', 'arrowright'])) {
				event.preventDefault();
				appState.goToEnd();
				return;
			}

			// Space for play/pause
			if (event.key === ' ' || event.code === 'Space') {
				event.preventDefault();
				appState.togglePlay();
				return;
			}
		}
	}

	function handleSelectExample(code: string) {
		appState.loadExample(code);
		examplesModalOpen = false;
	}

	onMount(() => {
		appState.initTheme();
		window.addEventListener('keydown', handleKeydown);
	});

	onDestroy(() => {
		appState.destroy();
		if (typeof window !== 'undefined') {
			window.removeEventListener('keydown', handleKeydown);
		}
	});
</script>

<!-- Islands layout: panels float on a canvas background -->
<div class="h-full flex flex-col gap-1.5 p-1.5 bg-islands-100 dark:bg-islands-950">
	<!-- TopBar island (z-20 to ensure dropdown renders above other panels) -->
	<div class="islands-panel relative z-20">
		<TopBar onOpenExamples={() => examplesModalOpen = true} />
	</div>

	<!-- Main content island -->
	<main class="flex-1 min-h-0 overflow-hidden relative z-10">
		{@render children()}
	</main>
</div>

<ExamplesModal
	open={examplesModalOpen}
	onClose={() => examplesModalOpen = false}
	onSelect={handleSelectExample}
/>
