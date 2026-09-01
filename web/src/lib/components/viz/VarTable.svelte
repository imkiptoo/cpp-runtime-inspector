<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import LocalRow from './LocalRow.svelte';

	interface Props {
		/** Render order for the rows. */
		names: string[];
		values: Record<string, EncodedValue>;
		sizes?: Record<string, number>;
		types?: Record<string, string>;
		addresses?: Record<string, string>;
		/** Show the Address column. Globals have no address, so it is off there. */
		showAddress?: boolean;
	}

	let { names, values, sizes, types, addresses, showAddress = false }: Props = $props();
</script>

<!-- Values can be wide (structs, long strings), so the table scrolls
     horizontally rather than clipping or squeezing the columns. -->
<div class="overflow-x-auto">
	<table class="w-full border-collapse font-mono text-[13px]">
		<thead>
			<tr class="text-[10px] uppercase tracking-wider text-islands-400 dark:text-islands-500">
				<th class="py-1 pr-3 text-left font-medium">Name</th>
				<th class="py-1 pr-3 text-left font-medium">Type</th>
				<th class="py-1 text-right font-medium">Value</th>
				{#if showAddress}
					<th class="py-1 pl-3 text-right font-medium">Address</th>
				{/if}
			</tr>
		</thead>
		<tbody>
			{#each names as name}
				<LocalRow
					{name}
					value={values[name]}
					size={sizes?.[name]}
					type={types?.[name]}
					address={addresses?.[name]}
					{showAddress}
				/>
			{/each}
		</tbody>
	</table>
</div>
