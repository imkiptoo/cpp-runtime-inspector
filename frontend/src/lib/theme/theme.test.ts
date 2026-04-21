import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
	resolveTheme,
	cycleTheme,
	getThemeIcon,
	getThemeLabel,
	type ThemePreference
} from './theme';

describe('theme/theme', () => {
	describe('resolveTheme', () => {
		it('returns light for light preference', () => {
			expect(resolveTheme('light')).toBe('light');
		});

		it('returns dark for dark preference', () => {
			expect(resolveTheme('dark')).toBe('dark');
		});

		it('returns system preference for system', () => {
			// Default mock returns false for prefers-color-scheme: dark
			expect(resolveTheme('system')).toBe('light');
		});
	});

	describe('cycleTheme', () => {
		it('cycles system -> light', () => {
			expect(cycleTheme('system')).toBe('light');
		});

		it('cycles light -> dark', () => {
			expect(cycleTheme('light')).toBe('dark');
		});

		it('cycles dark -> system', () => {
			expect(cycleTheme('dark')).toBe('system');
		});
	});

	describe('getThemeIcon', () => {
		it('returns correct icons', () => {
			expect(getThemeIcon('light')).toBe('sun');
			expect(getThemeIcon('dark')).toBe('moon');
			expect(getThemeIcon('system')).toBe('monitor');
		});
	});

	describe('getThemeLabel', () => {
		it('returns correct labels', () => {
			expect(getThemeLabel('light')).toBe('Light');
			expect(getThemeLabel('dark')).toBe('Dark');
			expect(getThemeLabel('system')).toBe('System');
		});
	});
});
