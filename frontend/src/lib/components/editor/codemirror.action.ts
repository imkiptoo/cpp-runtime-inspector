/**
 * CodeMirror editor setup with C++ support and trace line highlighting.
 */
import { EditorView, keymap, highlightActiveLine, highlightActiveLineGutter, drawSelection, dropCursor, rectangularSelection, crosshairCursor } from '@codemirror/view';
import { EditorState, StateField, StateEffect, Compartment } from '@codemirror/state';
import { defaultKeymap, history, historyKeymap, indentWithTab } from '@codemirror/commands';
import { syntaxHighlighting, defaultHighlightStyle, bracketMatching, foldGutter, indentOnInput } from '@codemirror/language';
import { cpp } from '@codemirror/lang-cpp';
import { closeBrackets, closeBracketsKeymap } from '@codemirror/autocomplete';
import { highlightSelectionMatches, searchKeymap } from '@codemirror/search';
import { lintKeymap } from '@codemirror/lint';
import { createTraceLineExtension } from '$lib/editor/traceLine';
import { createGutterClickExtension } from '$lib/editor/gutterJump';
import { lightTheme, darkTheme } from '$lib/editor/sppTheme';
import type { ResolvedTheme } from '$lib/theme/theme';

export interface EditorOptions {
	initialCode: string;
	onChange: (code: string) => void;
	onRun: () => void;
	onLineClick: (line: number) => void;
}

export interface EditorInstance {
	view: EditorView;
	setTraceLine: (line: number | null) => void;
	setTheme: (theme: ResolvedTheme) => void;
	setCode: (code: string) => void;
	getCode: () => string;
	destroy: () => void;
}

/**
 * Create a CodeMirror editor instance.
 */
export function createEditor(container: HTMLElement, options: EditorOptions): EditorInstance {
	const { initialCode, onChange, onRun, onLineClick } = options;

	// Theme compartment for dynamic switching
	const themeCompartment = new Compartment();

	// Trace line extension
	const { extension: traceLineExt, setTraceLine } = createTraceLineExtension();

	// Gutter click extension
	const gutterClickExt = createGutterClickExtension(onLineClick);

	// Run on Mod+Enter
	const runKeymap = keymap.of([
		{
			key: 'Mod-Enter',
			run: () => {
				onRun();
				return true;
			}
		}
	]);

	// Update listener for code changes
	const updateListener = EditorView.updateListener.of((update) => {
		if (update.docChanged) {
			onChange(update.state.doc.toString());
		}
	});

	const state = EditorState.create({
		doc: initialCode,
		extensions: [
			// Basic editing
			// Note: lineNumbers() removed - gutterJump.ts provides clickable line numbers
			highlightActiveLineGutter(),
			highlightActiveLine(),
			history(),
			foldGutter(),
			drawSelection(),
			dropCursor(),
			EditorState.allowMultipleSelections.of(true),
			indentOnInput(),
			bracketMatching(),
			closeBrackets(),
			rectangularSelection(),
			crosshairCursor(),
			highlightSelectionMatches(),

			// Keymaps
			keymap.of([
				...closeBracketsKeymap,
				...defaultKeymap,
				...searchKeymap,
				...historyKeymap,
				...lintKeymap,
				indentWithTab
			]),
			runKeymap,

			// C++ language
			cpp(),
			syntaxHighlighting(defaultHighlightStyle, { fallback: true }),

			// Custom extensions
			traceLineExt,
			gutterClickExt,

			// Theme (will be reconfigured)
			themeCompartment.of(lightTheme),

			// Change listener
			updateListener
		]
	});

	const view = new EditorView({
		state,
		parent: container
	});

	return {
		view,
		setTraceLine,
		setTheme: (theme: ResolvedTheme) => {
			view.dispatch({
				effects: themeCompartment.reconfigure(theme === 'dark' ? darkTheme : lightTheme)
			});
		},
		setCode: (code: string) => {
			const currentCode = view.state.doc.toString();
			if (currentCode !== code) {
				view.dispatch({
					changes: { from: 0, to: view.state.doc.length, insert: code }
				});
			}
		},
		getCode: () => view.state.doc.toString(),
		destroy: () => view.destroy()
	};
}
