/**
 * Media query hook for reactive media query matching.
 */

/**
 * Create a reactive media query matcher.
 */
export function createMediaQuery(query: string) {
	let matches = $state(false);

	if (typeof window !== 'undefined') {
		const mediaQuery = window.matchMedia(query);
		matches = mediaQuery.matches;

		const handler = (e: MediaQueryListEvent) => {
			matches = e.matches;
		};

		mediaQuery.addEventListener('change', handler);

		// Cleanup handled by Svelte's reactivity system
	}

	return {
		get matches() { return matches; }
	};
}

/**
 * Check if user prefers reduced motion.
 */
export function usePrefersReducedMotion() {
	return createMediaQuery('(prefers-reduced-motion: reduce)');
}

/**
 * Check if user prefers dark color scheme.
 */
export function usePrefersDark() {
	return createMediaQuery('(prefers-color-scheme: dark)');
}

/**
 * Check if viewport is mobile-sized.
 */
export function useIsMobile() {
	return createMediaQuery('(max-width: 768px)');
}
