/**
 * Context for heap addresses lookup.
 * Allows PointerChip and other components to get real memory addresses.
 */
import { getContext, setContext } from 'svelte';

const HEAP_ADDRESSES_KEY = Symbol('heap-addresses');

export interface HeapAddressesContext {
	getAddress(heapId: number | string): string | undefined;
}

export function setHeapAddressesContext(addresses: Record<string, string> | undefined): HeapAddressesContext {
	const ctx: HeapAddressesContext = {
		getAddress(heapId: number | string): string | undefined {
			if (!addresses) return undefined;
			return addresses[String(heapId)];
		}
	};
	setContext(HEAP_ADDRESSES_KEY, ctx);
	return ctx;
}

export function getHeapAddressesContext(): HeapAddressesContext | undefined {
	return getContext<HeapAddressesContext>(HEAP_ADDRESSES_KEY);
}
