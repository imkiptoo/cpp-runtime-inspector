/**
 * FLIP animation primitives for smooth layout transitions.
 * FLIP = First, Last, Invert, Play
 */

/** Duration of FLIP animations in ms */
export const FLIP_DURATION = 250;

export interface FlipRect {
	x: number;
	y: number;
	width: number;
	height: number;
}

export interface FlipState {
	id: string;
	rect: FlipRect;
	opacity: number;
}

export interface FlipTransition {
	id: string;
	from: FlipRect;
	to: FlipRect;
	deltaX: number;
	deltaY: number;
	scaleX: number;
	scaleY: number;
}

/**
 * Capture current positions of elements.
 */
export function capturePositions(
	container: HTMLElement,
	selector: string = '[data-flip-id]'
): Map<string, FlipState> {
	const positions = new Map<string, FlipState>();
	const elements = container.querySelectorAll(selector);

	elements.forEach((el) => {
		const id = (el as HTMLElement).dataset.flipId;
		if (!id) return;

		const rect = el.getBoundingClientRect();
		const containerRect = container.getBoundingClientRect();

		positions.set(id, {
			id,
			rect: {
				x: rect.left - containerRect.left,
				y: rect.top - containerRect.top,
				width: rect.width,
				height: rect.height
			},
			opacity: parseFloat(getComputedStyle(el).opacity) || 1
		});
	});

	return positions;
}

/**
 * Calculate transitions between first and last positions.
 */
export function calculateTransitions(
	first: Map<string, FlipState>,
	last: Map<string, FlipState>
): {
	moved: FlipTransition[];
	entered: string[];
	exited: string[];
} {
	const moved: FlipTransition[] = [];
	const entered: string[] = [];
	const exited: string[] = [];

	// Find moved and exited
	for (const [id, firstState] of first) {
		const lastState = last.get(id);
		if (lastState) {
			// Element moved
			const deltaX = firstState.rect.x - lastState.rect.x;
			const deltaY = firstState.rect.y - lastState.rect.y;
			const scaleX = firstState.rect.width / (lastState.rect.width || 1);
			const scaleY = firstState.rect.height / (lastState.rect.height || 1);

			if (Math.abs(deltaX) > 0.5 || Math.abs(deltaY) > 0.5 ||
			    Math.abs(scaleX - 1) > 0.01 || Math.abs(scaleY - 1) > 0.01) {
				moved.push({
					id,
					from: firstState.rect,
					to: lastState.rect,
					deltaX,
					deltaY,
					scaleX,
					scaleY
				});
			}
		} else {
			// Element exited
			exited.push(id);
		}
	}

	// Find entered
	for (const id of last.keys()) {
		if (!first.has(id)) {
			entered.push(id);
		}
	}

	return { moved, entered, exited };
}

/**
 * Apply FLIP animation to an element.
 */
export function animateFlip(
	element: HTMLElement,
	transition: FlipTransition,
	duration: number = 250,
	easing: string = 'cubic-bezier(0.16, 1, 0.3, 1)'
): Animation {
	// Invert: start from old position
	element.style.transform = `translate(${transition.deltaX}px, ${transition.deltaY}px) scale(${transition.scaleX}, ${transition.scaleY})`;
	element.style.transformOrigin = 'top left';

	// Play: animate to new position
	return element.animate(
		[
			{
				transform: `translate(${transition.deltaX}px, ${transition.deltaY}px) scale(${transition.scaleX}, ${transition.scaleY})`
			},
			{
				transform: 'translate(0, 0) scale(1, 1)'
			}
		],
		{
			duration,
			easing,
			fill: 'forwards'
		}
	);
}

/**
 * Apply enter animation to a new element.
 */
export function animateEnter(
	element: HTMLElement,
	duration: number = 200,
	easing: string = 'cubic-bezier(0.16, 1, 0.3, 1)'
): Animation {
	return element.animate(
		[
			{
				opacity: 0,
				transform: 'scale(0.9) translateY(8px)'
			},
			{
				opacity: 1,
				transform: 'scale(1) translateY(0)'
			}
		],
		{
			duration,
			easing,
			fill: 'forwards'
		}
	);
}

/**
 * Apply exit animation to a removed element.
 */
export function animateExit(
	element: HTMLElement,
	duration: number = 150,
	easing: string = 'ease-out'
): Animation {
	return element.animate(
		[
			{
				opacity: 1,
				transform: 'scale(1) translateY(0)'
			},
			{
				opacity: 0,
				transform: 'scale(0.9) translateY(-8px)'
			}
		],
		{
			duration,
			easing,
			fill: 'forwards'
		}
	);
}

/**
 * Orchestrate FLIP animation for a container.
 */
export async function performFlip(
	container: HTMLElement,
	updateFn: () => void | Promise<void>,
	options: {
		selector?: string;
		duration?: number;
		stagger?: number;
	} = {}
): Promise<void> {
	const { selector = '[data-flip-id]', duration = 250, stagger = 0 } = options;

	// First: capture positions
	const first = capturePositions(container, selector);

	// Update DOM
	await updateFn();

	// Last: capture new positions
	const last = capturePositions(container, selector);

	// Calculate transitions
	const { moved, entered } = calculateTransitions(first, last);

	// Animate moves
	const animations: Animation[] = [];
	moved.forEach((transition, index) => {
		const element = container.querySelector(`[data-flip-id="${transition.id}"]`);
		if (element instanceof HTMLElement) {
			const delay = stagger * index;
			setTimeout(() => {
				animations.push(animateFlip(element, transition, duration));
			}, delay);
		}
	});

	// Animate enters
	entered.forEach((id, index) => {
		const element = container.querySelector(`[data-flip-id="${id}"]`);
		if (element instanceof HTMLElement) {
			const delay = stagger * (moved.length + index);
			setTimeout(() => {
				animations.push(animateEnter(element, duration));
			}, delay);
		}
	});

	// Wait for all animations
	await Promise.all(animations.map((a) => a.finished));
}

/**
 * Create a FLIP context for managing animations across steps.
 */
export function createFlipContext() {
	let previousPositions: Map<string, FlipState> = new Map();

	return {
		/**
		 * Capture current state before a change.
		 */
		beforeUpdate(container: HTMLElement, selector?: string) {
			previousPositions = capturePositions(container, selector);
		},

		/**
		 * Animate after a change.
		 */
		afterUpdate(container: HTMLElement, selector?: string, duration?: number) {
			const current = capturePositions(container, selector);
			const { moved, entered } = calculateTransitions(previousPositions, current);

			// Animate
			for (const transition of moved) {
				const element = container.querySelector(`[data-flip-id="${transition.id}"]`);
				if (element instanceof HTMLElement) {
					animateFlip(element, transition, duration);
				}
			}

			for (const id of entered) {
				const element = container.querySelector(`[data-flip-id="${id}"]`);
				if (element instanceof HTMLElement) {
					animateEnter(element, duration);
				}
			}

			previousPositions = current;
		}
	};
}
