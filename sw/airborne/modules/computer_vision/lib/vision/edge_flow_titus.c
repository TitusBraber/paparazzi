/*
 * Copyright (C) 2016 Kimberly McGuire <k.n.mcguire@tudelft.nl
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file modules/computer_vision/lib/vision/edge_flow.c
 * @brief calculate optical flow with EdgeFlow
 *
 * Edge-histogram matching, implementation by K. N. McGuire
 * Publication: Local Histogram Matching for Efficient Optical Flow Computation Applied to Velocity Estimation on Pocket Drones
 * by K.N. McGuire et al. (2016), ICRA 2016
 */

#include <lib/vision/edge_flow_titus.h>
/**
 * Calc_previous_frame_nr; adaptive Time Horizon
 * @param[in] *opticflow The opticalflow structure
 * @param[in] *result The optical flow result
 * @param[in] *current_frame_nr: The current frame number of the circular array of the edge_hist struct
 * @param[in] *previous_frame_offset: previous frame offset of how far the method should compare the edgehistogram
 * @param[out] *previous_frame_nr: previous frame index of the edgehist struct
 */
void  calc_previous_frame_nr(struct opticflow_result_t *result, struct opticflow_t *opticflow, uint8_t current_frame_nr,
		uint8_t *previous_frame_offset, uint8_t *previous_frame_nr)
{
	// Adaptive Time Horizon:
	// if the flow measured in previous frame is small,
	// the algorithm will choose an frame further away back from the
	// current frame to detect subpixel flow
	if (MAX_HORIZON > 2) {

		uint32_t flow_mag_x, flow_mag_y;
		flow_mag_x = abs(result->flow_x);
		flow_mag_y = abs(result->flow_y);
		uint32_t min_flow = 3;
		uint32_t max_flow = opticflow->search_distance - 3;

		uint8_t previous_frame_offset_x = previous_frame_offset[0];
		uint8_t previous_frame_offset_y = previous_frame_offset[1];

		// IF statements which will decrement the previous frame offset
		// if the measured flow of last loop is higher than max value (higher flow measured)
		// and visa versa
		if (flow_mag_x > max_flow && previous_frame_offset_x > 1) {
			previous_frame_offset[0] = previous_frame_offset_x - 1;
		}
		if (flow_mag_x < min_flow && previous_frame_offset_x < MAX_HORIZON - 1) {
			previous_frame_offset[0] = previous_frame_offset_x + 1;
		}
		if (flow_mag_y > max_flow && previous_frame_offset_y > 1) {
			previous_frame_offset[1] = previous_frame_offset_y - 1;
		}
		if (flow_mag_y < min_flow && previous_frame_offset_y < MAX_HORIZON - 1) {
			previous_frame_offset[1] = previous_frame_offset_y + 1;
		}
	}

	//Wrap index previous frame offset from current frame nr.
	previous_frame_nr[0] = (current_frame_nr - previous_frame_offset[0] + MAX_HORIZON) %
			MAX_HORIZON;
	previous_frame_nr[1] = (current_frame_nr - previous_frame_offset[1] + MAX_HORIZON) %
			MAX_HORIZON;
}

/**
 * Calculate a edge/gradient histogram for each dimension of the image
 * @param[in] *img  The image frame to calculate the edge histogram from
 * @param[out] *edge_histogram  The edge histogram from the current frame_step
 * @param[in] direction  Indicating if the histogram is made in either x or y direction
 * @param[in] edge_threshold  A threshold if a gradient is considered a edge or not
 */
void calculate_edge_histogram(struct image_t *img, int32_t edge_histogram[],
		char direction, uint16_t edge_threshold)
{
	uint8_t *img_buf = (uint8_t *)img->buf;

	// TODO use arm_conv_q31()
	int32_t sobel_sum = 0;
	int32_t Sobel[3] = { -1, 0, 1};

	uint32_t y = 0, x = 0;
	int32_t c = 0;

	uint32_t idx = 0;

	uint16_t image_width = img->w;
	uint16_t image_height = img->h;
	uint32_t interlace;
	if (img->type == IMAGE_GRAYSCALE) {
		interlace = 1;
	} else {
		if (img->type == IMAGE_YUV422) {
			interlace = 2;
		} else
			while (1);   // hang to show user something isn't right
	}


	// compute edge histogram
	if (direction == 'x') {
		// set values that are not visited
		edge_histogram[0] = edge_histogram[image_width - 1] = 0;
		for (x = 1; x < image_width - 1; x++) {
			edge_histogram[x] = 0;
			for (y = 0; y < image_height; y++) {
				sobel_sum = 0;
				for (c = -1; c <= 1; c+=2) {
					idx = interlace * (image_width * y + (x + c));
					sobel_sum += Sobel[c + 1] * (int32_t)img_buf[idx];
				}
				sobel_sum = abs(sobel_sum);
				if (sobel_sum > edge_threshold) {
					edge_histogram[x] += sobel_sum;
				}
			}
		}
	} else if (direction == 'y') {
		// set values that are not visited
		edge_histogram[0] = edge_histogram[image_height - 1] = 0;
		for (y = 1; y < image_height - 1; y++) {
			edge_histogram[y] = 0;
			for (x = 0; x < image_width; x++) {
				sobel_sum = 0;

				for (c = -1; c <= 1; c+=2) {
					idx = interlace * (image_width * (y + c) + x);

					sobel_sum += Sobel[c + 1] * (int32_t)img_buf[idx];
				}
				sobel_sum = abs(sobel_sum);
				if (sobel_sum > edge_threshold) {
					edge_histogram[y] += sobel_sum;
				}
			}
		}
	} else
		while (1);  // hang to show user something isn't right
}

/**
 * Calculate_displacement calculates the displacement between two histograms
 * @param[in] *edge_histogram  The edge histogram from the current frame_step
 * @param[in] *edge_histogram_prev  The edge histogram from the previous frame_step
 * @param[out] *displacement array with pixel displacement of the sequential edge histograms
 * @param[in] size  Indicating the size of the displacement array
 * @param[in] window Indicating the search window size
 * @param[in] disp_range  Indicating the maximum disparity range for the block matching
 * @param[in] der_shift  The pixel shift estimated by the angle rate of the IMU
 */
void calculate_edge_displacement(int32_t *edge_histogram, int32_t *edge_histogram_prev, int32_t *displacement,
		uint16_t size,
		uint8_t window, uint8_t disp_range, int32_t der_shift)
{
	int32_t c = 0, r = 0;
	uint32_t x = 0;
	uint32_t SAD_temp[2 * DISP_RANGE_MAX + 1]; // size must be at least 2*D + 1

	int32_t W = window;
	int32_t D = disp_range;


	uint8_t SHIFT_TOO_FAR = 0;
	memset(displacement, 0, sizeof(int32_t)*size);

	int32_t border[2];
	if (der_shift < 0) {
		border[0] =  W + D + der_shift;
		border[1] = size - W - D;
	} else if (der_shift > 0) {
		border[0] =  W + D;
		border[1] = size - W - D - der_shift;
	} else {
		border[0] =  W + D;
		border[1] = size - W - D;
	}
	if (border[0] >= border[1] || abs(der_shift) >= 10) {
		SHIFT_TOO_FAR = 1;
	}
	// TODO: replace with arm offset subtract
	for (x = border[0]; x < border[1]; x++) {
		//			for (x = border[1]; x > border[0]; x--) {
		if (!SHIFT_TOO_FAR) {
			for (c = -D; c <= D; c++) {
				SAD_temp[c + D] = 0;
				for (r = -W; r <= W; r++) {
					SAD_temp[c + D] += abs(edge_histogram[x + r] - edge_histogram_prev[x + r + c + der_shift]);
				}
			}
			displacement[x] = (int32_t)getMinimum(SAD_temp, 2 * D + 1) - D;
		} else {
		}
	}
}

/**
 * Calculate minimum of an array
 * @param[in] *a Array containing values
 * @param[in] *n The size of the array
 * @return The index of the smallest value of the array
 */
uint32_t getMinimum(uint32_t *a, uint32_t n)
{
	uint32_t i;
	uint32_t min_ind1 = 0;
	uint32_t min_err = a[min_ind1];
	//	uint32_t min_err_tot = 0;
	for (i = 1; i < n; i++) {
		if (a[i] <= min_err) {
			min_ind1 = i;
			min_err = a[i];
			//			min_err_tot += min_err;
		}
	}
	return min_ind1;

	//	uint32_t min_ind2 = 0;
	//	min_err = a[min_ind2];
	////	uint32_t min_err_tot = 0;
	//	for (i = n; i > 1; i--) {
	//		if (a[i] <= min_err) {
	//			min_ind2 = i;
	//			min_err = a[i];
	////			min_err_tot += min_err;
	//		}
	//	}


	//	//*min_error = min_err_tot;
	//	if(min_ind1 == min_ind2)
	//	{
	//	return min_ind1;
	//	}
	//	else
	//	{
	//		printf("ind1: %d, ind2: %d, min_err1: %d, min_err2: %d\n",min_ind1,min_ind2,a[min_ind1],a[min_ind2]);
	//		return 999;
	//	}
}


/**
 * Fits a linear model to an array with pixel displacements with least squares
 * @param[in] *displacements Array with Pixel Displacements
 * @param[out] *divergence Global divergence of pixel displacements
 * @param[out] *flow  Global translational flow from pixel displacements
 * @param[in] *size Size of displacement array
 * @param[in] border  A border offset of the array that should not be considerd for the line fit
 * @param[in] RES  Resolution to be used for the integer based linefit
 */
void line_fit(int32_t *displacement, float *divergence, int32_t *flow, uint32_t size, uint32_t border,
		uint16_t RES)
{
	int32_t x;

	int32_t count = 0;
	int32_t sumY = 0;
	int32_t sumX = 0;
	int32_t sumX2 = 0;
	int32_t sumXY = 0;
	int32_t xMean = 0;
	int32_t yMean = 0;
	//	float count = 0;
	//	float sumY = 0;
	//	float sumX = 0;
	//	float sumX2 = 0;
	//	float sumXY = 0;
	//	float xMean = 0;
	//	float yMean = 0;
	int32_t divergence_int = 0;
	//	float divergence_float = 0;

	int32_t border_int = (int32_t)border;
	int32_t size_int = (int32_t)size;
	uint32_t total_error = 0;

	*divergence = 0;
	*flow = 0;

	// compute fixed sums
	int32_t xend = size_int - border_int - 1;
	sumX = xend * (xend + 1) / 2 - border_int * (border_int + 1) / 2 + border_int;
	sumX2 = xend * (xend + 1) * (2 * xend + 1) / 6 - border_int * (border_int + 1) * (2 * border_int+ 1) / 6 + border_int*border_int;
	xMean = (size_int - 1) / 2;
	count = size_int - 2 * border_int;

	for (x = border_int; x < size - border_int; x++) {
		sumY += displacement[x];
		sumXY += x * displacement[x];
	}

	yMean = RES * sumY / count;

	divergence_int = (RES * sumXY - sumX * yMean) / (sumX2 - sumX * xMean);    // compute slope of line ax + b
	//	divergence_float = (float) ( (RES * sumXY - sumX * yMean) / (sumX2 - sumX * xMean) );    // compute slope of line ax + b

	*divergence = divergence_int;
	//	*divergence = divergence_float;
	*flow = (int32_t)(yMean - *divergence * xMean);  // compute b (or y) intercept of line ax + b
	//	*flow = (float)(yMean - *divergence * xMean);  // compute b (or y) intercept of line ax + b

	for (x = border_int; x < size - border_int; x++) {
		//		total_error += (uint32_t)(abs(RES * displacement[x] - divergence_float * x + yMean));
		total_error += (uint32_t)(abs(RES * displacement[x] - divergence_int * x + yMean));
	}
	//		if(size==320)
	//		{
	//			printf("Regular \n");
	//
	//			printf("sumY: %d \n",sumY);
	//
	//			printf("RES: %d \n",RES);
	//			printf("sumXY: %d \n",sumXY);
	//			printf("sumX: %d \n",sumX);
	//			printf("yMean: %d \n",yMean);
	//			printf("sumX2: %d \n",sumX2);
	//			printf("xMean: %d \n",xMean);
	//
	//			printf("divergence_int: %d \n",divergence_int);
	//			printf("flow: %d \n",(yMean-divergence_int*xMean));
	//			printf("total_error: %d \n \n",total_error);
	//		}

	//	if(size==320)
	//	{
	//		printf("Regular \n");
	//
	//		printf("sumY: %f \n",sumY);
	//
	//		printf("RES: %d \n",RES);
	//		printf("sumXY: %f \n",sumXY);
	//		printf("sumX: %f \n",sumX);
	//		printf("yMean: %f \n",yMean);
	//		printf("sumX2: %f \n",sumX2);
	//		printf("xMean: %f \n",xMean);
	//
	//		printf("divergence_float: %f \n",divergence_float);
	//		printf("flow: %f \n",(yMean-divergence_float*xMean));
	//		printf("total_error: %d \n \n",total_error);
	//	}
}

/* weighted_line_fit: fits a line using least squares to the histogram disparity map, excluding the areas that have faulty distance measurements
 * \param displacement is an array that contains the pixel displacements of the compared edgehistograms
 * \param faulty_distance is an array with binary values, to indicate where the distance measure was faulty and not (those coordinates will not be included in the line fit)
 * \param divergence is slope of the optical flow field
 * \param slope is intercept of the optical flow (calculated from middle from image)
 * \param size is the  size of stereo_distance_per_column
 * \param border is the search window + search distance used in blockmatching
 * \param RES is resolution used to calculate the line fit (int based math)
 *
 * TODO: make the linefit so, that it can fit the line with uncertainty weights.
 * */
void weighted_line_fit(int32_t *displacement, uint8_t *faulty_distance,
		float *divergence, int32_t *flow, uint32_t size, uint32_t border,
		uint16_t RES)
{
	int32_t x;

	//	float count = 0;
	//	float sumY = 0;
	//	float sumX = 0;
	//	float sumX2 = 0;
	//	float sumXY = 0;
	//
	//	float xMean = 0;
	//	float yMean = 0;

	int32_t count = 0;
	int32_t sumY = 0;
	int32_t sumX = 0;
	int32_t sumX2 = 0;
	int32_t sumXY = 0;

	int32_t xMean = 0;
	int32_t yMean = 0;
	int32_t divergence_int = 0;
	//	float divergence_float = 0;

	int32_t border_int = (int32_t) border;
	int32_t size_int = (int32_t) size;
	uint32_t total_error = 0;

	*divergence = 0;
	*flow = 0;

	// compute fixed sums
	for (x = border_int; x < size_int - border_int; x++) {
		if (faulty_distance[x] == 0) {
			sumX += x;
			sumY += RES *  displacement[x];

			sumX2 += x * x;
			sumXY += x * displacement[x] * RES;

			count++;
		}
	}

	*divergence = 0;  //slope;
	*flow = 0;  //intercept;

	xMean = sumX / count;
	yMean = sumY / count;

	if ((sumX2 - sumX * xMean) != 0) { // preven seg fault
		divergence_int = (sumXY - sumX * yMean) / (sumX2 - sumX * xMean);  // compute slope of line ax + b
		//		divergence_float = (float) ((sumXY - sumX * yMean) / (sumX2 - sumX * xMean) );  // compute slope of line ax + b

		*divergence = divergence_int;
		//		*divergence = divergence_float;
	}

	*flow = (int32_t)(yMean - *divergence * xMean); // compute b (or y) intercept of line ax + b
	//	*flow = (float)(yMean - *divergence * xMean); // compute b (or y) intercept of line ax + b


	for (x = border_int; x < size_int - border_int; x++) {
		//				total_error += (uint32_t)(abs(RES * displacement[x] - divergence_float * x + yMean));
		total_error += (uint32_t)(abs(RES * displacement[x] - divergence_int * x + yMean));
	}
	//	if(size==320)
	//	{
	//		printf("Weighted \n");
	//
	//		printf("sumY: %d \n",sumY);
	//
	//		printf("RES: %d \n",RES);
	//		printf("sumXY: %d \n",sumXY);
	//		printf("sumX: %d \n",sumX);
	//		printf("yMean: %d \n",yMean);
	//		printf("sumX2: %d \n",sumX2);
	//		printf("xMean: %d \n",xMean);
	//
	//
	//		printf("divergence_int: %d \n",divergence_int);
	//		printf("flow: %d \n",(yMean-divergence_int*xMean));
	//		printf("total_error: %d \n \n",total_error);
	//	}
	//		if(size==320)
	//		{
	//			printf("Weighted \n");
	//
	//			printf("sumY: %f \n",sumY);
	//
	//			printf("RES: %d \n",RES);
	//			printf("sumXY: %f \n",sumXY);
	//			printf("sumX: %f \n",sumX);
	//			printf("yMean: %f \n",yMean);
	//			printf("sumX2: %f \n",sumX2);
	//			printf("xMean: %f \n",xMean);
	//
	//			printf("divergence_float: %f \n",divergence_float);
	//			printf("flow: %f \n",(yMean-divergence_float*xMean));
	//			printf("total_error: %d \n \n",total_error);
	//		}
}
/* weighted_line_fit: fits a line using least squares to the histogram disparity map, excluding the areas that have faulty distance measurements
 * \param displacement is an array that contains the pixel displacements of the compared edgehistograms
 * \param divergence is slope of the optical flow field
 * \param slope is intercept of the optical flow (calculated from middle from image)
 * \param faulty_distance is an array with binary values, to indicate where the distance measure was faulty and not (those coordinates will not be included in the line fit)
 * \param size is the  size of stereo_distance_per_column
 * \param border is the search window + search distance used in blockmatching
 * \param RES is resolution used to calculate the line fit (int based math)
 *
 * TODO: Make the inlier_threshold and inlier ratio adaptable
 * */
void line_fit_RANSAC(int32_t *displacement, float *divergence, int32_t *flow,
		uint16_t size, uint32_t border, int32_t RES)
{

	int16_t inlier_threshold = 2000;
	int16_t inlier_ratio = 40;
	int16_t num_inliers_wanted = (int16_t)(inlier_ratio
			* (size - (int32_t) border * 2) / 100);
	int16_t num_inliers = 0;
	//Fit a linear line with RANSAC (from Guido's code)
	int32_t ransac_iter = 50;
	int32_t it;
	uint32_t ind1, ind2, tmp, entry;
	int32_t total_error = 0, best_ind = 0;
	int32_t error;
	int32_t dx, dflow, predicted_flow;
	// flow = a * x + b
	int32_t a[ransac_iter];
	int32_t b[ransac_iter];
	uint32_t errors[ransac_iter];

	uint16_t  counter_pass_check = 0;

	uint16_t entries = size - 2 * border;

	for (it = 0; it < ransac_iter; it++) {
		ind1 = rand() % entries + border;
		ind2 = rand() % entries + border;

		while (ind1 == ind2) {
			ind2 = rand() % entries + border;
		}
		// TODO: is this really necessary?
		if (ind1 > ind2) {
			tmp = ind2;
			ind2 = ind1;
			ind1 = tmp;
		}

		dx = ind2 - ind1;   // never zero
		dflow = displacement[ind2] - displacement[ind1];

		// Fit line with two points
		a[it] = RES * dflow / dx;
		b[it] = RES * (int32_t)displacement[ind1] - (a[it] * ind1);
		// evaluate fit:

		total_error = 0;
		for (entry = border; entry < size - border; entry++) {
			predicted_flow = a[it] * entry + b[it];
			error = abs((RES * (int32_t)displacement[entry] - predicted_flow));

			if ((int32_t) error < inlier_threshold * RES) {
				num_inliers++;
				total_error += (error / (RES));
			}

			//total_error += ipow(RES*displacement[entry] - predicted_flow,2);
		}


		if ((num_inliers > num_inliers_wanted)) {
			errors[it] = total_error;
			counter_pass_check++;

		} else {

			errors[it] = UINT32_MAX;
		}

		num_inliers = 0;
		total_error = 0;
	}

	// select best fit:
	uint32_t min_error = 0;
	best_ind = getMinimum2(errors, ransac_iter, &min_error);

	if (counter_pass_check > 0) {
		*divergence = a[best_ind];
		*flow = b[best_ind];
	} else {
		*divergence = 0;
		*flow = 0;
	}
}

/**
 * Draws edgehistogram, displacement and linefit directly on the image for debugging (only for edgeflow in horizontal direction!!)
 * @param[out] *img The image structure where will be drawn on
 * @param[in] edgeflow Information structure for flow information
 * @param[in] Displacement Pixel wise Displacement array
 * @param[in] *edge_hist_x Horizontal edge_histogram
 */
void draw_edgeflow_img(struct image_t *img, struct edge_flow_t edgeflow, int32_t *edge_hist_x_prev
		, int32_t *edge_hist_x)
{
	struct point_t point1;
	struct point_t point2;
	struct point_t point1_prev;
	struct point_t point2_prev;
	struct point_t point1_extra;
	struct point_t point2_extra;
	uint16_t i;

	for (i = 1; i < img->w - 1; i++) {
		point1.y = -(uint16_t)edge_hist_x[i] / 100 + img->h / 3;
		point1.x = i;
		point2.y = -(uint16_t)edge_hist_x[i + 1] / 100 + img->h / 3;
		point2.x = i + 1;

		point1_prev.y = -(uint16_t)edge_hist_x_prev[i] / 100  + img->h * 2 / 3;
		point1_prev.x = i;
		point2_prev.y = -(uint16_t)edge_hist_x_prev[i + 1] / 100 + img->h * 2 / 3;
		point2_prev.x = i + 1;

		image_draw_line(img, &point1, &point2);
		image_draw_line(img, &point1_prev, &point2_prev);
	}

	//	point1_extra.y = (edgeflow.flow_x + edgeflow.div_x * img->w / 2) / 100 + img->h / 2;
	//	point1_extra.x = 0;
	//	point2_extra.y = (edgeflow.flow_x + edgeflow.div_x * img->w / 2) / 100 + img->h / 2;
	//	point2_extra.x = img->w;
	//	image_draw_line(img, &point1_extra, &point2_extra);

	point1_extra.y = edgeflow.flow_x + (img->h / 2);
	point1_extra.x = 0;
	point2_extra.y = (edgeflow.flow_x + edgeflow.div_x * img->w) + (img->h / 2);
	point2_extra.x = img->w;
	image_draw_line(img, &point1_extra, &point2_extra);

}

/**
 * getAmountPeaks, calculates the amount of peaks in a edge histogram
 * @param[in] *edgehist Horizontal edge_histogram
 * @param[in] thres The threshold from which a peak is considered significant peak or not
 * @param[in] size  Size of the array
 * @param[return] amount of peaks
 */
uint32_t getAmountPeaks(int32_t *edgehist, uint32_t thres, int32_t size)
{
	uint32_t  amountPeaks = 0;
	uint32_t i = 0;

	for (i = 1; i < size + 1;  i ++) {
		if (edgehist[i - 1] < edgehist[i] && edgehist[i] > edgehist[i + 1] && edgehist[i] > thres) {
			amountPeaks ++;
		}
	}
	return amountPeaks;
}
/* getMinimum2: finds minimum value in array
 * \param a is an array containing the values
 * \param n is size of the array
 * \param min_error is the minimum value of array
 * \return min_ind is the index of the minimum value located on the array
 *
 * */
uint32_t getMinimum2(uint32_t *a, uint32_t n, uint32_t *min_error)
{
	uint32_t i;
	uint32_t min_ind = 0;
	*min_error = a[min_ind];
	for (i = 1; i < n; i++) {
		if (a[i] < *min_error) {
			min_ind = i;
			*min_error = a[i];
		}
	}
	return min_ind;
}