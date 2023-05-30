#pragma once

#include "QuickSort.h"

/*
 * Sort the elements of array inplace. This is called insertion sort.
 * Thus it is stable, consumes 1 additional element, time complexity:
 * best case is O(n), average case is O(n*n), worst case is O(n*n).
 * arr: The array to be sorted.
 * arr_index_sup: The index in array until the elements have to be sorted.
 * arr_index_start: The index of first element in array to sort.
 * Return: nothing.
 */
template<typename T>
void InsertionSort(T arr[], const int arr_index_sup, const int arr_index_start = 0)
{
	int i, j;
	const auto arr_index_sup_local = arr_index_sup - 1;
	for (i = arr_index_start; i < arr_index_sup_local; i++)
	{
		const T key = arr[i + 1];
		// Move elements of arr[arr_index_start..i], that are greater than key,
		// to one position ahead of their current position.
		for (j = i; j >= arr_index_start && arr[j] > key; j--)
			arr[j + 1] = arr[j];
		if (j < i)
			arr[j + 1] = key;
	}
}

/*
 * Merge the elements of sorted array into the sorted inplace array in a
 * way that the result inplace array remains sorted.
 * If an element in array also exists in inplace array then it is considered
 * greater thus it will be stored after the equal element of inplace array.
 * The array and inplace array must be sorted incremented.
 * arr: The sorted array of which elements are merged into inplace array.
 * arr_index_sup: The index in array until the elements have to be merged.
 * inplace_arr: The sorted array where the elements are merged.
 * inplace_arr_size: The current size of inplace array.
 * inplace_arr_sup: The index in inplace array which is next to last element.
 *                  With other words it is the maximum size of inplace array.
 * arr_index_start: The index of first element in array to merge.
 * Return: the new size of inplace array (>= 0) or negative n on failure, where
 * n is the required more space for inplace array to grow.
 */
template<typename T>
inline int MergeFromSortedToSortedInplace(T arr[], const int arr_index_sup,
	T inplace_arr[], const int inplace_arr_size, const int inplace_arr_sup, const int arr_index_start = 0)
{
	if (arr_index_start >= arr_index_sup)
		return inplace_arr_size;
	// Inserting the new elements from arr to inplace arr by going backwards.
	auto inplace_arr_new_size = inplace_arr_size + arr_index_sup - arr_index_start;
	if (inplace_arr_new_size > inplace_arr_sup)
		return inplace_arr_sup - inplace_arr_new_size; // Negative means error, value means required more space.
	auto inplace_arr_new_index = inplace_arr_new_size;
	auto arr_index = arr_index_sup;
	inplace_arr_index = inplace_arr_size;
	while (inplace_arr_index > 0 && arr_index > arr_index_start)
		inplace_arr[--inplace_arr_new_index] = arr[arr_index] >= arr[inplace_arr_index] ? arr[--arr_index] : arr[--inplace_arr_index];
	// Copying the remaining arr elements.
	while (arr_index > arr_index_start)
		inplace_arr[--inplace_arr_new_index] = arr[--arr_index];
	return inplace_arr_new_size;
}

/*
 * Merge the new elements of sorted array into the sorted inplace array in a
 * way that the result inplace array remains sorted.
 * New element in array means not existing element in inplace array.
 * If an element in array is multicated in array then the later elements are
 * considered greater thus only the first element of multicates will be stored
 * in inplace array.
 * The array and inplace array must be sorted incremented.
 * arr: The sorted array of which new elements are merged into inplace array.
 * arr_index_sup: The index in array until the elements have to be merged.
 * inplace_arr: The sorted array where the new elements are merged.
 * inplace_arr_size: The current size of inplace array.
 * inplace_arr_sup: The index in inplace array which is next to last element.
 *                  With other words it is the maximum size of inplace array.
 * arr_index_start: The index of first element in array to merge.
 * Return: the new size of inplace array (>= 0) or negative n on failure, where
 * n is the required more space for inplace array to grow.
 */
template<typename T>
inline int MergeNewFromSortedToSortedInplace(T arr[], const int arr_index_sup,
	T inplace_arr[], const int inplace_arr_size, const int inplace_arr_sup,
	const int arr_index_start = 0)
{
	if (arr_index_start >= arr_index_sup)
		return inplace_arr_size;
	// Count how many new elements are there and have to be inserted.
	auto new_inplace_elements = 0;
	auto inplace_arr_index = 0;
	for (int arr_index = arr_index_start; arr_index < arr_index_sup; arr_index++)
	{
		const auto& arr_element = arr[arr_index];
//		KdPrint(("MNFSTSI(): exploring #%d. scanned %hhu (%ld)\n",
//			arr_index, arr_element.chrn.sector, arr_element.abstime));
		if (arr_index > arr_index_start && arr[arr_index - 1] == arr_element)
			continue; // The array element is duplicated thus it was just processed so skip it.
		// Check if array element exists in the sorted inplace arr.
		for (; inplace_arr_index < inplace_arr_size && inplace_arr[inplace_arr_index] < arr_element; inplace_arr_index++)
			;
		if (inplace_arr_index < inplace_arr_size)
//			KdPrint(("MNFSTSI(): comparing: #%d. scanned %hhu (%ld) vs #%d. first %hhu (%ld)\n",
//				arr_index, arr_element.chrn.sector, arr_element.abstime,
//				inplace_arr_index, inplace_arr[inplace_arr_index].chrn.sector, inplace_arr[inplace_arr_index].abstime));
		if (inplace_arr_index < inplace_arr_size && inplace_arr[inplace_arr_index] == arr_element)
			continue; // The array element exists so skip it.
		new_inplace_elements++;
	}
//	KdPrint(("MNFSTSI(): new_inplace_elements %d\n", new_inplace_elements));
	if (new_inplace_elements == 0) // No new element so inplace array is ready.
		return inplace_arr_size;
	// Inserting the new elements from array to inplace array by going backwards.
	auto inplace_arr_new_size = inplace_arr_size + new_inplace_elements;
	if (inplace_arr_new_size > inplace_arr_sup)
		return inplace_arr_sup - inplace_arr_new_size; // Negative means error, value means required more space.
	auto inplace_arr_new_index = inplace_arr_new_size - 1;
	auto arr_index = arr_index_sup - 1;
	inplace_arr_index = inplace_arr_size - 1;
	while (inplace_arr_index >= 0 && arr_index >= arr_index_start)
	{
		const auto& arr_element = arr[arr_index];
//		KdPrint(("MNFSTSI(): merging #%d.: #%d. scanned %hhu (%ld) vs #%d. first %hhu (%ld)\n",
//			inplace_arr_new_index, arr_index, arr_element.chrn.sector, arr_element.abstime,
//			inplace_arr_index, inplace_arr[inplace_arr_index].chrn.sector, inplace_arr[inplace_arr_index].abstime));
		if (arr_element != inplace_arr[inplace_arr_index] // The array element does not exist in inplace array
			&& (arr_index <= arr_index_start || arr[arr_index - 1] != arr_element)) // and the array element is not multicated.
		{
			if (arr_element >= inplace_arr[inplace_arr_index]) // = is impossible but >= looks like the expression in MergeFromSortedToSortedInplace.
			{
				inplace_arr[inplace_arr_new_index] = arr_element;
				arr_index--;
			}
			else
				inplace_arr[inplace_arr_new_index] = inplace_arr[inplace_arr_index--];
			inplace_arr_new_index--;
		}
		else // Skipping equal (existing) element.
			--arr_index;
	}
	// Copying the remaining array elements except multicates.
	while (arr_index >= arr_index_start)
	{
		const auto& arr_element = arr[arr_index];
//		KdPrint(("MNFSTSI(): remainmerging #%d.: #%d. scanned %hhu (%ld)\n",
//			inplace_arr_new_index, arr_index, arr_element.chrn.sector, arr_element.abstime));
		if (arr_index <= arr_index_start || arr[arr_index - 1] != arr_element) // The array element is not multicated.
			inplace_arr[inplace_arr_new_index--] = arr_element;
		arr_index--;
	}
	return inplace_arr_new_size;
}
