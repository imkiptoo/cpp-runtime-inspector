/**
 * CodeMirror themes for light and dark modes.
 * Inspired by JetBrains Islands UI design language.
 */
import { EditorView } from '@codemirror/view';
import { HighlightStyle, syntaxHighlighting } from '@codemirror/language';
import { tags } from '@lezer/highlight';

// Islands Light theme colors
const lightColors = {
	background: '#ffffff',
	foreground: '#1E1F22', // islands-900
	cursor: '#3574F0', // brand-500
	selection: 'rgba(53, 116, 240, 0.2)', // brand-500 with opacity
	selectionMatch: 'rgba(53, 116, 240, 0.1)',
	lineHighlight: '#F7F8FA', // islands-50
	gutterBackground: '#F7F8FA', // islands-50
	gutterForeground: '#9DA0A8', // islands-400
	gutterActiveForeground: '#4E5157', // islands-600
	gutterBorder: '#EBECF0' // islands-100
};

// Islands Dark theme colors
const darkColors = {
	background: '#1E1F22', // islands-900
	foreground: '#DFE1E5', // islands-200
	cursor: '#3574F0', // brand-500
	selection: 'rgba(53, 116, 240, 0.35)',
	selectionMatch: 'rgba(53, 116, 240, 0.2)',
	lineHighlight: '#2B2D30', // islands-800
	gutterBackground: '#1E1F22', // islands-900
	gutterForeground: '#6E7179', // islands-500
	gutterActiveForeground: '#9DA0A8', // islands-400
	gutterBorder: '#393B40' // islands-700
};

// JetBrains-inspired syntax colors (Light)
const syntaxLight = {
	keyword: '#0033B3', // JetBrains blue for keywords
	controlKeyword: '#0033B3',
	operator: '#1E1F22',
	number: '#1750EB', // Blue for numbers
	string: '#067D17', // Green for strings
	character: '#067D17',
	bool: '#0033B3',
	null: '#0033B3',
	comment: '#8C8C8C', // Gray for comments
	function: '#00627A', // Teal for functions
	variable: '#1E1F22',
	type: '#000000', // Black for types (bold)
	class: '#000000',
	namespace: '#000000',
	macro: '#871094', // Purple for macros/preprocessor
	property: '#871094',
	attribute: '#174AD4',
	label: '#00627A',
	invalid: '#FF0000'
};

// JetBrains-inspired syntax colors (Dark - Darcula-like)
const syntaxDark = {
	keyword: '#CC7832', // Orange for keywords (Darcula style)
	controlKeyword: '#CC7832',
	operator: '#A9B7C6',
	number: '#6897BB', // Blue for numbers
	string: '#6A8759', // Green for strings
	character: '#6A8759',
	bool: '#CC7832',
	null: '#CC7832',
	comment: '#808080', // Gray for comments
	function: '#FFC66D', // Yellow for functions
	variable: '#A9B7C6',
	type: '#A9B7C6', // Light gray for types
	class: '#A9B7C6',
	namespace: '#A9B7C6',
	macro: '#9876AA', // Purple for macros
	property: '#9876AA',
	attribute: '#BABABA',
	label: '#FFC66D',
	invalid: '#FF6B68'
};

// Base theme styles
function createBaseTheme(colors: typeof lightColors, isDark: boolean) {
	return EditorView.theme(
		{
			'&': {
				backgroundColor: colors.background,
				color: colors.foreground,
				height: '100%'
			},
			'.cm-content': {
				caretColor: colors.cursor,
				fontFamily: "'JetBrains Mono', Menlo, Monaco, Consolas, monospace",
				fontSize: '13px',
				lineHeight: '1.65',
				padding: '8px 0'
			},
			'.cm-cursor, .cm-dropCursor': {
				borderLeftColor: colors.cursor,
				borderLeftWidth: '2px'
			},
			'&.cm-focused .cm-selectionBackground, .cm-selectionBackground, .cm-content ::selection': {
				backgroundColor: colors.selection
			},
			'.cm-selectionMatch': {
				backgroundColor: colors.selectionMatch
			},
			'.cm-activeLine': {
				backgroundColor: colors.lineHighlight
			},
			'.cm-gutters': {
				backgroundColor: colors.gutterBackground,
				color: colors.gutterForeground,
				borderRight: `1px solid ${colors.gutterBorder}`,
				paddingRight: '4px'
			},
			'.cm-activeLineGutter': {
				backgroundColor: colors.lineHighlight,
				color: colors.gutterActiveForeground
			},
			'.cm-lineNumbers .cm-gutterElement': {
				padding: '0 8px 0 12px',
				minWidth: '36px',
				fontSize: '12px'
			},
			'.cm-foldGutter .cm-gutterElement': {
				padding: '0 4px'
			},
			'.cm-scroller': {
				overflow: 'auto'
			},
			// Matching brackets
			'&.cm-focused .cm-matchingBracket': {
				backgroundColor: isDark ? 'rgba(53, 116, 240, 0.25)' : 'rgba(53, 116, 240, 0.15)',
				outline: `1px solid ${isDark ? '#3574F0' : '#3574F0'}`
			},
			// Search highlight
			'.cm-searchMatch': {
				backgroundColor: isDark ? 'rgba(255, 193, 7, 0.3)' : 'rgba(255, 193, 7, 0.4)',
				outline: `1px solid ${isDark ? '#FFC107' : '#FFA000'}`
			},
			'.cm-searchMatch.cm-searchMatch-selected': {
				backgroundColor: isDark ? 'rgba(255, 193, 7, 0.5)' : 'rgba(255, 193, 7, 0.6)'
			}
		},
		{ dark: isDark }
	);
}

// Light syntax highlighting (JetBrains IntelliJ Light style)
const lightHighlightStyle = HighlightStyle.define([
	{ tag: tags.keyword, color: syntaxLight.keyword, fontWeight: '600' },
	{ tag: tags.controlKeyword, color: syntaxLight.controlKeyword, fontWeight: '600' },
	{ tag: tags.operator, color: syntaxLight.operator },
	{ tag: tags.number, color: syntaxLight.number },
	{ tag: tags.string, color: syntaxLight.string },
	{ tag: tags.character, color: syntaxLight.character },
	{ tag: tags.bool, color: syntaxLight.bool, fontWeight: '600' },
	{ tag: tags.null, color: syntaxLight.null, fontWeight: '600' },
	{ tag: tags.comment, color: syntaxLight.comment, fontStyle: 'italic' },
	{ tag: tags.lineComment, color: syntaxLight.comment, fontStyle: 'italic' },
	{ tag: tags.blockComment, color: syntaxLight.comment, fontStyle: 'italic' },
	{ tag: tags.function(tags.variableName), color: syntaxLight.function },
	{ tag: tags.definition(tags.variableName), color: syntaxLight.variable },
	{ tag: tags.typeName, color: syntaxLight.type, fontWeight: '500' },
	{ tag: tags.className, color: syntaxLight.class, fontWeight: '500' },
	{ tag: tags.namespace, color: syntaxLight.namespace },
	{ tag: tags.macroName, color: syntaxLight.macro },
	{ tag: tags.propertyName, color: syntaxLight.property },
	{ tag: tags.attributeName, color: syntaxLight.attribute },
	{ tag: tags.labelName, color: syntaxLight.label },
	{ tag: tags.heading, color: syntaxLight.variable, fontWeight: 'bold' },
	{ tag: tags.emphasis, fontStyle: 'italic' },
	{ tag: tags.strong, fontWeight: 'bold' },
	{ tag: tags.link, color: syntaxLight.function, textDecoration: 'underline' },
	{ tag: tags.invalid, color: syntaxLight.invalid, textDecoration: 'wavy underline' },
	// C++ specific
	{ tag: tags.meta, color: syntaxLight.macro }, // Preprocessor directives
	{ tag: tags.processingInstruction, color: syntaxLight.macro }
]);

// Dark syntax highlighting (JetBrains Darcula style)
const darkHighlightStyle = HighlightStyle.define([
	{ tag: tags.keyword, color: syntaxDark.keyword, fontWeight: '500' },
	{ tag: tags.controlKeyword, color: syntaxDark.controlKeyword, fontWeight: '500' },
	{ tag: tags.operator, color: syntaxDark.operator },
	{ tag: tags.number, color: syntaxDark.number },
	{ tag: tags.string, color: syntaxDark.string },
	{ tag: tags.character, color: syntaxDark.character },
	{ tag: tags.bool, color: syntaxDark.bool, fontWeight: '500' },
	{ tag: tags.null, color: syntaxDark.null, fontWeight: '500' },
	{ tag: tags.comment, color: syntaxDark.comment, fontStyle: 'italic' },
	{ tag: tags.lineComment, color: syntaxDark.comment, fontStyle: 'italic' },
	{ tag: tags.blockComment, color: syntaxDark.comment, fontStyle: 'italic' },
	{ tag: tags.function(tags.variableName), color: syntaxDark.function },
	{ tag: tags.definition(tags.variableName), color: syntaxDark.variable },
	{ tag: tags.typeName, color: syntaxDark.type },
	{ tag: tags.className, color: syntaxDark.class },
	{ tag: tags.namespace, color: syntaxDark.namespace },
	{ tag: tags.macroName, color: syntaxDark.macro },
	{ tag: tags.propertyName, color: syntaxDark.property },
	{ tag: tags.attributeName, color: syntaxDark.attribute },
	{ tag: tags.labelName, color: syntaxDark.label },
	{ tag: tags.heading, color: syntaxDark.variable, fontWeight: 'bold' },
	{ tag: tags.emphasis, fontStyle: 'italic' },
	{ tag: tags.strong, fontWeight: 'bold' },
	{ tag: tags.link, color: syntaxDark.function, textDecoration: 'underline' },
	{ tag: tags.invalid, color: syntaxDark.invalid, textDecoration: 'wavy underline' },
	// C++ specific
	{ tag: tags.meta, color: syntaxDark.macro }, // Preprocessor directives
	{ tag: tags.processingInstruction, color: syntaxDark.macro }
]);

// Export complete themes
export const lightTheme = [createBaseTheme(lightColors, false), syntaxHighlighting(lightHighlightStyle)];

export const darkTheme = [createBaseTheme(darkColors, true), syntaxHighlighting(darkHighlightStyle)];
