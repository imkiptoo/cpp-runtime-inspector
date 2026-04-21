import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
	detectPlatform,
	getModifierKey,
	getModifierKeyCode,
	isModifierPressed,
	formatShortcut,
	matchesShortcut
} from './kbd';

describe('platform/kbd', () => {
	describe('detectPlatform', () => {
		const originalNavigator = global.navigator;

		afterEach(() => {
			Object.defineProperty(global, 'navigator', {
				value: originalNavigator,
				writable: true
			});
		});

		it('returns unknown when navigator is undefined', () => {
			// @ts-expect-error - testing undefined navigator
			global.navigator = undefined;
			expect(detectPlatform()).toBe('unknown');
		});

		it('detects mac platform', () => {
			// @ts-expect-error - mock navigator
			global.navigator = {
				userAgent: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)',
				platform: 'MacIntel'
			};
			expect(detectPlatform()).toBe('mac');
		});

		it('detects windows platform', () => {
			// @ts-expect-error - mock navigator
			global.navigator = {
				userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)',
				platform: 'Win32'
			};
			expect(detectPlatform()).toBe('windows');
		});

		it('detects linux platform', () => {
			// @ts-expect-error - mock navigator
			global.navigator = {
				userAgent: 'Mozilla/5.0 (X11; Linux x86_64)',
				platform: 'Linux x86_64'
			};
			expect(detectPlatform()).toBe('linux');
		});
	});

	describe('getModifierKey', () => {
		it('returns ⌘ for mac', () => {
			expect(getModifierKey('mac')).toBe('⌘');
		});

		it('returns Ctrl for windows', () => {
			expect(getModifierKey('windows')).toBe('Ctrl');
		});

		it('returns Ctrl for linux', () => {
			expect(getModifierKey('linux')).toBe('Ctrl');
		});

		it('returns Ctrl for unknown', () => {
			expect(getModifierKey('unknown')).toBe('Ctrl');
		});
	});

	describe('getModifierKeyCode', () => {
		it('returns metaKey for mac', () => {
			expect(getModifierKeyCode('mac')).toBe('metaKey');
		});

		it('returns ctrlKey for windows', () => {
			expect(getModifierKeyCode('windows')).toBe('ctrlKey');
		});

		it('returns ctrlKey for linux', () => {
			expect(getModifierKeyCode('linux')).toBe('ctrlKey');
		});
	});

	describe('isModifierPressed', () => {
		it('checks metaKey for mac', () => {
			const event = { metaKey: true, ctrlKey: false } as KeyboardEvent;
			expect(isModifierPressed(event, 'mac')).toBe(true);
		});

		it('checks ctrlKey for windows', () => {
			const event = { metaKey: false, ctrlKey: true } as KeyboardEvent;
			expect(isModifierPressed(event, 'windows')).toBe(true);
		});

		it('returns false when modifier not pressed', () => {
			const event = { metaKey: false, ctrlKey: false } as KeyboardEvent;
			expect(isModifierPressed(event, 'mac')).toBe(false);
			expect(isModifierPressed(event, 'windows')).toBe(false);
		});
	});

	describe('formatShortcut', () => {
		it('formats mod key for mac', () => {
			expect(formatShortcut(['mod', 'k'], 'mac')).toBe('⌘K');
		});

		it('formats mod key for windows', () => {
			expect(formatShortcut(['mod', 'k'], 'windows')).toBe('Ctrl+K');
		});

		it('formats shift for mac', () => {
			expect(formatShortcut(['shift', 'a'], 'mac')).toBe('⇧A');
		});

		it('formats shift for windows', () => {
			expect(formatShortcut(['shift', 'a'], 'windows')).toBe('Shift+A');
		});

		it('formats alt/option for mac', () => {
			expect(formatShortcut(['alt', 'x'], 'mac')).toBe('⌥X');
			expect(formatShortcut(['option', 'x'], 'mac')).toBe('⌥X');
		});

		it('formats alt for windows', () => {
			expect(formatShortcut(['alt', 'x'], 'windows')).toBe('Alt+X');
		});

		it('formats enter key', () => {
			expect(formatShortcut(['mod', 'enter'], 'mac')).toBe('⌘↵');
			expect(formatShortcut(['mod', 'enter'], 'windows')).toBe('Ctrl+Enter');
		});

		it('formats arrow keys', () => {
			expect(formatShortcut(['up'], 'mac')).toBe('↑');
			expect(formatShortcut(['down'], 'mac')).toBe('↓');
			expect(formatShortcut(['left'], 'mac')).toBe('←');
			expect(formatShortcut(['right'], 'mac')).toBe('→');
		});

		it('formats escape key', () => {
			expect(formatShortcut(['esc'], 'mac')).toBe('Esc');
			expect(formatShortcut(['escape'], 'windows')).toBe('Esc');
		});

		it('formats space key', () => {
			expect(formatShortcut(['space'], 'mac')).toBe('Space');
		});

		it('formats tab key', () => {
			expect(formatShortcut(['tab'], 'mac')).toBe('⇥');
			expect(formatShortcut(['tab'], 'windows')).toBe('Tab');
		});

		it('formats backspace and delete', () => {
			expect(formatShortcut(['backspace'], 'mac')).toBe('⌫');
			expect(formatShortcut(['delete'], 'mac')).toBe('⌦');
			expect(formatShortcut(['backspace'], 'windows')).toBe('Backspace');
			expect(formatShortcut(['delete'], 'windows')).toBe('Delete');
		});

		it('uppercases regular keys', () => {
			expect(formatShortcut(['a'], 'mac')).toBe('A');
			expect(formatShortcut(['f1'], 'mac')).toBe('F1');
		});
	});

	describe('matchesShortcut', () => {
		it('matches mod+key on mac', () => {
			const event = {
				key: 'k',
				metaKey: true,
				ctrlKey: false,
				shiftKey: false,
				altKey: false
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['mod', 'k'], 'mac')).toBe(true);
		});

		it('matches ctrl+key on windows', () => {
			const event = {
				key: 'k',
				metaKey: false,
				ctrlKey: true,
				shiftKey: false,
				altKey: false
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['mod', 'k'], 'windows')).toBe(true);
		});

		it('matches shift modifier', () => {
			const event = {
				key: 'a',
				metaKey: false,
				ctrlKey: false,
				shiftKey: true,
				altKey: false
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['shift', 'a'], 'mac')).toBe(true);
		});

		it('matches alt modifier', () => {
			const event = {
				key: 'x',
				metaKey: false,
				ctrlKey: false,
				shiftKey: false,
				altKey: true
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['alt', 'x'], 'mac')).toBe(true);
		});

		it('returns false when modifier missing', () => {
			const event = {
				key: 'k',
				metaKey: false,
				ctrlKey: false,
				shiftKey: false,
				altKey: false
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['mod', 'k'], 'mac')).toBe(false);
		});

		it('returns false when key does not match', () => {
			const event = {
				key: 'j',
				metaKey: true,
				ctrlKey: false,
				shiftKey: false,
				altKey: false
			} as KeyboardEvent;
			expect(matchesShortcut(event, ['mod', 'k'], 'mac')).toBe(false);
		});
	});
});
