<script lang="ts">
	import { onMount } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import { createCodeViewer, type ViewerLanguage } from './codeViewer.action';

	interface Props {
		doc: string;
		language: ViewerLanguage;
	}

	let { doc, language }: Props = $props();

	let container: HTMLDivElement | undefined = $state();
	let viewer: ReturnType<typeof createCodeViewer> | undefined = $state();

	onMount(() => {
		if (!container) return;

		viewer = createCodeViewer(container, {
			doc,
			language,
			theme: appState.resolvedTheme
		});

		return () => {
			viewer?.destroy();
			viewer = undefined;
		};
	});

	// Refresh contents when a new run produces a different document.
	$effect(() => {
		viewer?.setDoc(doc);
	});

	// Follow the app theme, same as the source editor.
	$effect(() => {
		viewer?.setTheme(appState.resolvedTheme);
	});
</script>

<div class="h-full overflow-hidden" bind:this={container}></div>
