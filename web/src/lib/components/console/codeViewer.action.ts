/**
 * Read-only CodeMirror viewer for the console's Instrumented / JSON tabs.
 *
 * Shares the editor's theme and font (`sppTheme`) so these panels look like
 * the source pane, but drops everything to do with editing: no history, no
 * input handling, no autocomplete. Selection, search and copy still work.
 *
 * CodeMirror renders only the visible viewport, so this handles the large
 * documents these tabs produce far better than a single <pre>.
 */
import { EditorView, keymap, lineNumbers, highlightActiveLine, highlightActiveLineGutter, drawSelection } from '@codemirror/view';
import { EditorState, Compartment } from '@codemirror/state';
import { defaultKeymap } from '@codemirror/commands';
import { bracketMatching } from '@codemirror/language';
import { highlightSelectionMatches, searchKeymap } from '@codemirror/search';
import { cpp } from '@codemirror/lang-cpp';
import { json } from '@codemirror/lang-json';
import { lightTheme, darkTheme } from '$lib/editor/sppTheme';
import type { ResolvedTheme } from '$lib/theme/theme';

export type ViewerLanguage = 'cpp' | 'json';

export interface CodeViewerOptions {
	doc: string;
	language: ViewerLanguage;
	theme: ResolvedTheme;
}

export interface CodeViewerInstance {
	view: EditorView;
	setDoc: (doc: string) => void;
	setTheme: (theme: ResolvedTheme) => void;
	destroy: () => void;
}

function languageExtension(language: ViewerLanguage) {
	return language === 'json' ? json() : cpp();
}

/**
 * Mount a read-only viewer into `container`.
 */
export function createCodeViewer(
	container: HTMLElement,
	options: CodeViewerOptions
): CodeViewerInstance {
	const { doc, language, theme } = options;

	const themeCompartment = new Compartment();

	const state = EditorState.create({
		doc,
		extensions: [
			lineNumbers(),
			highlightActiveLineGutter(),
			highlightActiveLine(),
			drawSelection(),
			bracketMatching(),
			highlightSelectionMatches(),
			EditorState.allowMultipleSelections.of(true),

			// Read-only, but still focusable so the text can be selected and
			// searched. `readOnly` blocks edits; `editable` hides the cursor.
			EditorState.readOnly.of(true),
			EditorView.editable.of(false),

			// Long lines wrap rather than forcing horizontal scroll of the pane.
			EditorView.lineWrapping,

			languageExtension(language),

			// Navigation and search only — no editing or history bindings.
			keymap.of([
				...defaultKeymap.filter((b) => b.key !== 'Mod-Enter'),
				...searchKeymap
			]),

			themeCompartment.of(theme === 'dark' ? darkTheme : lightTheme)
		]
	});

	const view = new EditorView({ state, parent: container });

	return {
		view,
		setDoc: (next: string) => {
			if (view.state.doc.toString() === next) return;
			view.dispatch({
				changes: { from: 0, to: view.state.doc.length, insert: next }
			});
		},
		setTheme: (next: ResolvedTheme) => {
			view.dispatch({
				effects: themeCompartment.reconfigure(next === 'dark' ? darkTheme : lightTheme)
			});
		},
		destroy: () => view.destroy()
	};
}
