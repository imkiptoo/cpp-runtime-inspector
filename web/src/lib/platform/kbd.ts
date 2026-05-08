/**
 * Platform-specific keyboard hint utilities.
 */

export type Platform = 'mac' | 'windows' | 'linux' | 'unknown';

/**
 * Detect current platform.
 */
export function detectPlatform(): Platform {
	if (typeof navigator === 'undefined') return 'unknown';

	const ua = navigator.userAgent.toLowerCase();
	const platform = navigator.platform?.toLowerCase() ?? '';

	if (platform.includes('mac') || ua.includes('mac')) return 'mac';
	if (platform.includes('win') || ua.includes('win')) return 'windows';
	if (platform.includes('linux') || ua.includes('linux')) return 'linux';

	return 'unknown';
}

/**
 * Get modifier key name for current platform.
 */
export function getModifierKey(platform?: Platform): string {
	const p = platform ?? detectPlatform();
	return p === 'mac' ? '⌘' : 'Ctrl';
}

/**
 * Get modifier key code for event matching.
 */
export function getModifierKeyCode(platform?: Platform): 'metaKey' | 'ctrlKey' {
	const p = platform ?? detectPlatform();
	return p === 'mac' ? 'metaKey' : 'ctrlKey';
}

/**
 * Check if modifier key is pressed in event.
 */
export function isModifierPressed(event: KeyboardEvent | MouseEvent, platform?: Platform): boolean {
	const key = getModifierKeyCode(platform);
	return event[key];
}

/**
 * Format a keyboard shortcut for display.
 */
export function formatShortcut(
	keys: string[],
	platform?: Platform
): string {
	const p = platform ?? detectPlatform();
	const isMac = p === 'mac';

	return keys
		.map((key) => {
			switch (key.toLowerCase()) {
				case 'mod':
				case 'cmd':
				case 'ctrl':
					return isMac ? '⌘' : 'Ctrl';
				case 'shift':
					return isMac ? '⇧' : 'Shift';
				case 'alt':
				case 'option':
					return isMac ? '⌥' : 'Alt';
				case 'enter':
				case 'return':
					return isMac ? '↵' : 'Enter';
				case 'backspace':
					return isMac ? '⌫' : 'Backspace';
				case 'delete':
					return isMac ? '⌦' : 'Delete';
				case 'escape':
				case 'esc':
					return 'Esc';
				case 'tab':
					return isMac ? '⇥' : 'Tab';
				case 'space':
					return 'Space';
				case 'arrowup':
				case 'up':
					return '↑';
				case 'arrowdown':
				case 'down':
					return '↓';
				case 'arrowleft':
				case 'left':
					return '←';
				case 'arrowright':
				case 'right':
					return '→';
				default:
					return key.toUpperCase();
			}
		})
		.join(isMac ? ' ' : ' + ');
}

/**
 * Common keyboard shortcuts with platform-specific display.
 */
export const shortcuts = {
	run: () => formatShortcut(['mod', 'enter']),
	examples: () => formatShortcut(['mod', 'k']),
	stepForward: () => formatShortcut(['right']),
	stepBackward: () => formatShortcut(['left']),
	playPause: () => formatShortcut(['space']),
	goToStart: () => formatShortcut(['mod', 'left']),
	goToEnd: () => formatShortcut(['mod', 'right'])
};

/**
 * Check if an event matches a shortcut.
 */
export function matchesShortcut(
	event: KeyboardEvent,
	keys: string[],
	platform?: Platform
): boolean {
	const p = platform ?? detectPlatform();
	const modKey = getModifierKeyCode(p);

	for (const key of keys) {
		const k = key.toLowerCase();
		switch (k) {
			case 'mod':
			case 'cmd':
			case 'ctrl':
				if (!event[modKey]) return false;
				break;
			case 'shift':
				if (!event.shiftKey) return false;
				break;
			case 'alt':
			case 'option':
				if (!event.altKey) return false;
				break;
			default:
				if (event.key.toLowerCase() !== k) return false;
		}
	}

	return true;
}
