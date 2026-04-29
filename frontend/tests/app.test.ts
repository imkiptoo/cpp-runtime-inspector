import { test, expect } from '@playwright/test';

test.describe('C++ Runtime Inspector', () => {
	test.beforeEach(async ({ page }) => {
		await page.goto('/');
	});

	test('loads the application', async ({ page }) => {
		// Check title
		await expect(page).toHaveTitle('C++ Runtime Inspector');

		// Check brand is visible
		await expect(page.getByText('Runtime Inspector')).toBeVisible();

		// Check Run button exists
		await expect(page.getByRole('button', { name: /run/i })).toBeVisible();
	});

	test('has code editor with default code', async ({ page }) => {
		// Check CodeMirror editor exists
		const editor = page.locator('.cm-editor');
		await expect(editor).toBeVisible();

		// Check default code is loaded
		const content = await editor.locator('.cm-content').textContent();
		expect(content).toContain('int main()');
	});

	test('opens examples modal via button click', async ({ page }) => {
		// Click the examples button in the top bar
		await page.getByRole('button', { name: /examples/i }).click();

		// Check modal is open
		await expect(page.getByRole('dialog')).toBeVisible({ timeout: 5000 });

		// Check the modal title
		const modalTitle = page.locator('dialog h2');
		await expect(modalTitle).toContainText('Examples');

		// Check example items are visible
		await expect(page.locator('button:has-text("Hello World")')).toBeVisible();
		await expect(page.locator('button:has-text("Pointers Basics")')).toBeVisible();

		// Close with the X button
		await page.getByLabel('Close modal').click();
		await expect(page.getByRole('dialog')).not.toBeVisible();
	});

	test('selects example from modal', async ({ page }) => {
		// Open examples modal by clicking on top bar examples button
		await page.getByRole('button', { name: /examples/i }).click();

		// Wait for modal to be visible
		await expect(page.getByRole('dialog')).toBeVisible({ timeout: 5000 });

		// Find and click the Hello World example button using a more specific selector
		// The button contains both title and description, so we target the text-left button
		const exampleButton = page.locator('dialog button.text-left').filter({ hasText: 'Hello World' }).first();
		await expect(exampleButton).toBeVisible();

		// Use force click to ensure it triggers
		await exampleButton.click({ force: true });

		// Modal should close
		await expect(page.getByRole('dialog')).not.toBeVisible({ timeout: 3000 });

		// Wait for the editor content to change
		const editor = page.locator('.cm-editor .cm-content');
		await expect(editor).toContainText('Hello, World', { timeout: 5000 });
	});

	test('toggles theme', async ({ page }) => {
		// Open settings
		await page.getByRole('button', { name: /settings/i }).click();

		// Check settings menu is visible
		await expect(page.getByText('Theme')).toBeVisible();

		// Click dark theme
		await page.getByRole('button', { name: 'Dark' }).click();

		// Check dark class is applied
		await expect(page.locator('html')).toHaveClass(/dark/);

		// Click light theme
		await page.getByRole('button', { name: 'Light' }).click();

		// Check dark class is removed
		await expect(page.locator('html')).not.toHaveClass(/dark/);
	});

	test('shows visualization pane placeholder', async ({ page }) => {
		// Before running, should show placeholder with run hint
		await expect(page.getByText('to visualize.')).toBeVisible();
	});

	test('transport controls are disabled without trace', async ({ page }) => {
		// Step buttons should be disabled
		const stepForward = page.getByRole('button', { name: /step forward/i });
		const stepBackward = page.getByRole('button', { name: /step backward/i });

		await expect(stepForward).toBeDisabled();
		await expect(stepBackward).toBeDisabled();
	});

	test('console can be toggled', async ({ page }) => {
		// Find console toggle button
		const toggleButton = page.getByRole('button', { name: /toggle console/i });

		// Initially console may or may not be open
		const consolePane = page.locator('.console-output').first();

		// Toggle console
		await toggleButton.click();

		// Toggle again
		await toggleButton.click();
	});

	test('keyboard navigation works', async ({ page }) => {
		// These tests check that keyboard handlers don't throw errors
		// Actual functionality depends on having a trace loaded

		await page.keyboard.press('ArrowRight');
		await page.keyboard.press('ArrowLeft');
		await page.keyboard.press('Space');

		// App should still be responsive
		await expect(page.getByRole('button', { name: /run/i })).toBeVisible();
	});

	test('responsive layout', async ({ page }) => {
		// Test at mobile width
		await page.setViewportSize({ width: 375, height: 667 });

		// Core elements should still be visible
		await expect(page.locator('.cm-editor')).toBeVisible();
		await expect(page.getByRole('button', { name: /run/i })).toBeVisible();

		// Test at desktop width
		await page.setViewportSize({ width: 1280, height: 800 });

		// Brand text should be visible on desktop
		await expect(page.getByText('Runtime Inspector')).toBeVisible();
	});

	test('settings menu closes on outside click', async ({ page }) => {
		// Open settings
		await page.getByRole('button', { name: /settings/i }).click();

		// Settings should be visible
		await expect(page.getByText('Pointer Arrows')).toBeVisible();

		// Click outside (on the main content area)
		await page.locator('.cm-editor').click();

		// Settings should close (may take a moment)
		await expect(page.getByText('Pointer Arrows')).not.toBeVisible();
	});
});

// These tests require a running backend
test.describe('With Backend', () => {
	test.skip(({ browserName }) => true, 'Requires running backend');

	test('runs code and displays trace', async ({ page }) => {
		await page.goto('/');

		// Click Run button
		await page.getByRole('button', { name: /run/i }).click();

		// Wait for trace to load
		await expect(page.getByText('Compiling and tracing...')).toBeVisible();

		// Eventually should show visualization
		await expect(page.getByText('Stack')).toBeVisible({ timeout: 30000 });
	});

	test('step navigation works with trace', async ({ page }) => {
		await page.goto('/');

		// Run the code
		await page.getByRole('button', { name: /run/i }).click();

		// Wait for trace
		await expect(page.getByText('Stack')).toBeVisible({ timeout: 30000 });

		// Step forward should work
		const stepForward = page.getByRole('button', { name: /step forward/i });
		await expect(stepForward).not.toBeDisabled();
		await stepForward.click();

		// Step counter should update
		await expect(page.getByText(/2 \//)).toBeVisible();
	});
});
