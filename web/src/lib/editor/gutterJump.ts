/**
 * CodeMirror extension for clickable line numbers to jump in trace.
 */
import { gutter, GutterMarker } from '@codemirror/view';
import type { Extension } from '@codemirror/state';

/**
 * Create a gutter click extension.
 */
export function createGutterClickExtension(onLineClick: (line: number) => void): Extension {
	return gutter({
		class: 'cm-lineNumbers',
		lineMarker(view, line) {
			const lineNumber = view.state.doc.lineAt(line.from).number;
			return new ClickableLineMarker(lineNumber, onLineClick);
		},
		lineMarkerChange: () => false
	});
}

class ClickableLineMarker extends GutterMarker {
	constructor(
		private lineNumber: number,
		private onClick: (line: number) => void
	) {
		super();
	}

	toDOM() {
		const el = document.createElement('span');
		el.textContent = String(this.lineNumber);
		el.className = 'cm-gutterElement';
		el.style.cursor = 'pointer';
		el.addEventListener('click', (e) => {
			e.preventDefault();
			e.stopPropagation();
			this.onClick(this.lineNumber);
		});
		return el;
	}
}
