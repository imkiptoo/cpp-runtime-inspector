/**
 * Reactive theme application using Svelte 5 runes.
 */
import { subscribeToSystemTheme, resolveTheme, applyTheme, type ThemePreference, type ResolvedTheme } from './theme';

/**
 * Create a reactive theme manager.
 */
export function createThemeManager() {
	let preference = $state<ThemePreference>('system');
	let resolved = $state<ResolvedTheme>('light');
	let unsubscribe: (() => void) | null = null;

	function updateResolved() {
		resolved = resolveTheme(preference);
		applyTheme(resolved);
	}

	function setPreference(pref: ThemePreference) {
		preference = pref;
		updateResolved();
	}

	function init() {
		updateResolved();

		// Subscribe to system theme changes
		unsubscribe = subscribeToSystemTheme(() => {
			if (preference === 'system') {
				updateResolved();
			}
		});
	}

	function destroy() {
		if (unsubscribe) {
			unsubscribe();
			unsubscribe = null;
		}
	}

	return {
		get preference() { return preference; },
		get resolved() { return resolved; },
		setPreference,
		init,
		destroy
	};
}
