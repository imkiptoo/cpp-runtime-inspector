/**
 * Theme preference logic for dark/light/system mode.
 */

export type ThemePreference = 'light' | 'dark' | 'system';
export type ResolvedTheme = 'light' | 'dark';

const STORAGE_KEY = 'theme-preference';

/**
 * Get stored theme preference from localStorage.
 */
export function getStoredPreference(): ThemePreference {
	if (typeof localStorage === 'undefined') return 'system';

	const stored = localStorage.getItem(STORAGE_KEY);
	if (stored === 'light' || stored === 'dark' || stored === 'system') {
		return stored;
	}
	return 'system';
}

/**
 * Store theme preference to localStorage.
 */
export function setStoredPreference(preference: ThemePreference): void {
	if (typeof localStorage === 'undefined') return;
	localStorage.setItem(STORAGE_KEY, preference);
}

/**
 * Get system color scheme preference.
 */
export function getSystemPreference(): ResolvedTheme {
	if (typeof window === 'undefined') return 'light';

	return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

/**
 * Resolve theme preference to actual theme.
 */
export function resolveTheme(preference: ThemePreference): ResolvedTheme {
	if (preference === 'system') {
		return getSystemPreference();
	}
	return preference;
}

/**
 * Apply theme to document.
 */
export function applyTheme(theme: ResolvedTheme): void {
	if (typeof document === 'undefined') return;

	document.documentElement.classList.toggle('dark', theme === 'dark');
}

/**
 * Subscribe to system theme changes.
 */
export function subscribeToSystemTheme(callback: (theme: ResolvedTheme) => void): () => void {
	if (typeof window === 'undefined') return () => {};

	const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
	const handler = (e: MediaQueryListEvent) => {
		callback(e.matches ? 'dark' : 'light');
	};

	mediaQuery.addEventListener('change', handler);
	return () => mediaQuery.removeEventListener('change', handler);
}

/**
 * Initialize theme from stored preference.
 */
export function initializeTheme(): ResolvedTheme {
	const preference = getStoredPreference();
	const resolved = resolveTheme(preference);
	applyTheme(resolved);
	return resolved;
}

/**
 * Cycle through theme preferences.
 */
export function cycleTheme(current: ThemePreference): ThemePreference {
	const order: ThemePreference[] = ['system', 'light', 'dark'];
	const currentIndex = order.indexOf(current);
	return order[(currentIndex + 1) % order.length];
}

/**
 * Get icon name for theme preference.
 */
export function getThemeIcon(preference: ThemePreference): string {
	switch (preference) {
		case 'light':
			return 'sun';
		case 'dark':
			return 'moon';
		case 'system':
			return 'monitor';
	}
}

/**
 * Get label for theme preference.
 */
export function getThemeLabel(preference: ThemePreference): string {
	switch (preference) {
		case 'light':
			return 'Light';
		case 'dark':
			return 'Dark';
		case 'system':
			return 'System';
	}
}
