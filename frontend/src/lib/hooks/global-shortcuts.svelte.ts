/**
 * Global keyboard shortcuts hook.
 */
import { onMount, onDestroy } from 'svelte';
import { matchesShortcut } from '$lib/platform/kbd';

export interface ShortcutConfig {
	keys: string[];
	handler: () => void;
	/** If true, shortcut works even when in editor */
	global?: boolean;
}

/**
 * Create global keyboard shortcut handler.
 */
export function useGlobalShortcuts(shortcuts: ShortcutConfig[]) {
	function handleKeydown(event: KeyboardEvent) {
		// Check if we're in an input or editor
		const target = event.target as HTMLElement;
		const inInput = target.tagName === 'INPUT' || target.tagName === 'TEXTAREA';
		const inEditor = target.closest('.cm-editor') !== null;
		const inModal = target.closest('dialog[open]') !== null;

		for (const shortcut of shortcuts) {
			// Skip non-global shortcuts when in input/editor
			if (!shortcut.global && (inInput || inEditor)) {
				continue;
			}

			if (matchesShortcut(event, shortcut.keys)) {
				event.preventDefault();
				shortcut.handler();
				return;
			}
		}
	}

	onMount(() => {
		window.addEventListener('keydown', handleKeydown);
	});

	onDestroy(() => {
		window.removeEventListener('keydown', handleKeydown);
	});
}
