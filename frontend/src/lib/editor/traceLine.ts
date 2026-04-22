/**
 * CodeMirror extension for highlighting the current trace line.
 */
import { StateField, StateEffect, type Extension } from '@codemirror/state';
import { Decoration, type DecorationSet, EditorView } from '@codemirror/view';

// Effect to update the highlighted line
const setTraceLineEffect = StateEffect.define<number | null>();

// Line decoration
const traceLineDecoration = Decoration.line({ class: 'trace-line-highlight' });

// State field to track the highlighted line
const traceLineField = StateField.define<DecorationSet>({
	create() {
		return Decoration.none;
	},
	update(decorations, tr) {
		for (const effect of tr.effects) {
			if (effect.is(setTraceLineEffect)) {
				const line = effect.value;
				if (line === null || line <= 0) {
					return Decoration.none;
				}

				// Get the line in the document (1-indexed)
				const doc = tr.state.doc;
				if (line > doc.lines) {
					return Decoration.none;
				}

				const lineInfo = doc.line(line);
				return Decoration.set([traceLineDecoration.range(lineInfo.from)]);
			}
		}
		return decorations.map(tr.changes);
	},
	provide: (f) => EditorView.decorations.from(f)
});

/**
 * Create the trace line extension with a setter function.
 */
export function createTraceLineExtension(): {
	extension: Extension;
	setTraceLine: (line: number | null) => void;
} {
	let view: EditorView | null = null;

	const captureView = EditorView.updateListener.of((update) => {
		view = update.view;
	});

	return {
		extension: [traceLineField, captureView],
		setTraceLine: (line: number | null) => {
			if (view) {
				view.dispatch({
					effects: setTraceLineEffect.of(line)
				});

				// Scroll to line if not visible
				if (line !== null && line > 0 && line <= view.state.doc.lines) {
					const lineInfo = view.state.doc.line(line);
					view.dispatch({
						effects: EditorView.scrollIntoView(lineInfo.from, {
							y: 'center'
						})
					});
				}
			}
		}
	};
}
