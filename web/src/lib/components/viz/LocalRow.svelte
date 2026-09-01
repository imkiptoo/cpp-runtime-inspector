<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import { isPointerLike, isCAddress, getPointerType } from '$lib/trace/schema';
	import ValueChip from './ValueChip.svelte';

	interface Props {
		name: string;
		value: EncodedValue;
		size?: number;
		/** Declared type spelling from the runtime, e.g. "unsigned long". */
		type?: string;
		/** Storage address as hex, e.g. "0x16f37e888". Absent for globals. */
		address?: string;
		/** Render the address cell (off for globals, which have no address). */
		showAddress?: boolean;
	}

	let { name, value, size, type, address, showAddress = false }: Props = $props();

	// Prefer the type the runtime actually recorded. The fallback infers a type
	// from the JSON value shape, which is lossy — a `double` holding 42.0 is
	// indistinguishable from an `int` there — so it is only used when the
	// backend predates local_types/global_types.
	const typeLabel = $derived.by(() => {
		if (type) return type;

		if (isPointerLike(value)) {
			if (isCAddress(value)) {
				return getPointerType(value) || 'pointer';
			}
			return 'pointer';
		}
		if (typeof value === 'number') {
			return Number.isInteger(value) ? 'int' : 'double';
		}
		if (typeof value === 'boolean') return 'bool';
		if (typeof value === 'string') {
			if (value.length === 1) return 'char';
			return 'string';
		}
		return null;
	});
</script>

<tr
	class="border-t border-islands-100 dark:border-islands-700/50 align-baseline"
	data-testid="local-{name}"
>
	<!-- Name -->
	<td class="py-1.5 pr-3 whitespace-nowrap text-islands-800 dark:text-islands-200">
		{name}
	</td>

	<!-- Type (+ size) -->
	<td class="py-1.5 pr-3 whitespace-nowrap text-islands-400 dark:text-islands-500">
		{#if typeLabel}<span>{typeLabel}</span>{:else}<span>—</span>{/if}
		{#if size !== undefined}
			<span class="text-[11px]">({size}B)</span>
		{/if}
	</td>

	<!-- Value -->
	<td class="py-1.5 text-right">
		<ValueChip {value} />
	</td>

	<!-- Address -->
	{#if showAddress}
		<td
			class="py-1.5 pl-3 whitespace-nowrap text-right text-[11px] text-islands-400 dark:text-islands-500"
			title={address ?? 'address unavailable'}
		>
			{address ?? '—'}
		</td>
	{/if}
</tr>
