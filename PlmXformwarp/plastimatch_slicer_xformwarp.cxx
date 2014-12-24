/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include "plm_config.h"
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <time.h>
#include "plastimatch_slicer_xformwarpCLP.h"

#include "rt_study.h"
#include "rt_study_warp.h"
#include "warp_parms.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    bool have_img_input = false;
    bool have_ss_img_input = false;
    bool have_img_output = false;
    bool have_ss_img_output = false;
    Warp_parms parms;
    Plm_file_format file_type;
    Rt_study rtds;

    /* Input image (to set the size) */
    if (plmslc_xformwarp_reference_vol != "" 
	&& plmslc_xformwarp_reference_vol != "None")
    {
	parms.fixed_img_fn = plmslc_xformwarp_reference_vol.c_str();
    }

    /* Input image or dose */
    if (plmslc_xformwarp_input_img != "" 
	&& plmslc_xformwarp_input_img != "None")
    {
	have_img_input = true;
	parms.input_fn = plmslc_xformwarp_input_img.c_str();
    }

    /* Input ss image */
    if (plmslc_xformwarp_input_ss_img != "" 
	&& plmslc_xformwarp_input_ss_img != "None")
    {
	have_ss_img_input = true;
	parms.input_ss_img_fn = plmslc_xformwarp_input_ss_img.c_str();
    }

    if (!have_img_input && !have_ss_img_input) {
	printf ("Error.  No input specified.\n");
	return EXIT_FAILURE;
    }

    /* Get xform either from MRML scene or file */
    if (plmslc_xformwarp_input_xform_s != "" 
	&& plmslc_xformwarp_input_xform_s != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_xform_s.c_str();
    }
    else if (plmslc_xformwarp_input_vf_s != "" 
	&& plmslc_xformwarp_input_vf_s != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_vf_s.c_str();
    }
    else if (plmslc_xformwarp_input_xform_f != "" 
	&& plmslc_xformwarp_input_xform_f != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_xform_f.c_str();
    }

    printf ("xf_in_fn = %s\n", (const char*) parms.xf_in_fn);

    /* Output warped image or dose */
    if (plmslc_xformwarp_output_img != "" 
	&& plmslc_xformwarp_output_img != "None")
    {
	have_img_output = true;
	parms.output_img_fn = plmslc_xformwarp_output_img.c_str();
    }

    /* Output type */
    if (plmslc_xformwarp_output_type != "auto") {
	parms.output_type = plm_image_type_parse (
	    plmslc_xformwarp_output_type.c_str());
    }

    /* Output warped ss image */
    if (plmslc_xformwarp_output_ss_img != "" 
	&& plmslc_xformwarp_output_ss_img != "None")
    {
	have_ss_img_output = true;
	parms.output_ss_img_fn = plmslc_xformwarp_output_ss_img.c_str();
    }

    if (!(have_img_input && have_img_output)
	&& !(have_ss_img_input && have_ss_img_output))
    {
	printf ("Error.  Output images not specified correctly.\n");
	return EXIT_FAILURE;
    }

    /* What is the input file type? */
    file_type = plm_file_format_deduce ((const char*) parms.input_fn);

    /* Process warp */
    rt_study_warp (&rtds, file_type, &parms);

    return EXIT_SUCCESS;
}
