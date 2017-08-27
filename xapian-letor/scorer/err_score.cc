/** @file err_score.cc
 *  @brief Implementation of ERRScore
 *
 *  ERR Score is adapted from the paper: http://olivier.chapelle.cc/pub/err.pdf
 *  Chapelle, Metzler, Zhang, Grinspan (2009)
 *  Expected Reciprocal Rank for Graded Relevance
 */
/* Copyright (C) 2014 Hanxiao Sun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <config.h>

#include "xapian-letor/scorer.h"

#include "debuglog.h"

#include <algorithm>

using namespace std;

using namespace Xapian;

ERRScore::ERRScore()
{
    LOGCALL_CTOR(API, "ERRScore", NO_ARGS);
}

ERRScore::~ERRScore()
{
    LOGCALL_DTOR(API, "ERRScore");
}

double
ERRScore::score(const std::vector<FeatureVector> & fvv) const
{
    LOGCALL(API, double, "ERRScore::score", fvv);

    /* 2^(gmax) where gmax is the max of all the relevance grades.
     * since a 5 point grade sytem is used, possible relevance grades are
     * {0, 1, 2, 3, 4}, gmax = 4. For more information refer to section 4 of
     * the paper http://olivier.chapelle.cc/pub/err.pdf
     */
    int MAX_PROB = 16;

    int length = fvv.size();

    /* used to store values which change from labels to
     * satisfaction probability
     */
    double intermediate_values[length];
    double max_label = fvv[0].get_label();

    // store the labels set the by the user in intermediate_values.
    for (int i = 0; i < length; ++i) {
	double label = fvv[i].get_label();
	intermediate_values[i] = label;
	max_label = max(max_label, label);
    }

    // normalize the labels to an integer from 0 to 4 because we are using
    // a grade point system where each document is graded from 0 to 4.
    for (int i = 0; i < length; ++i) {
	intermediate_values[i] = round((intermediate_values[i] * 4) /
				       max_label);
    }

    // compute the satisfaction probability for label of each doc in the ranking.
    for (int i = 0; i < length; ++i) {
	intermediate_values[i] = (pow(2, intermediate_values[i]) - 1) /
				  MAX_PROB;
    }

    double p = 1;
    double err_score = 0;

    /* compute the accumulated probability p for each doc
     * that the user will stop at.
     * err_score = summation for each doc
     * ((satisfaction probability * p) / rank)
     */
    for (int i = 0; i < length; ++i) {
	err_score = err_score + (intermediate_values[i] * p / (i + 1));
	p = p * (1 - intermediate_values[i]);
    }

    return err_score;
}
