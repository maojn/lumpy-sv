/*****************************************************************************
 * SV_PairReader.cpp
 * (c) 2012 - Ryan M. Layer
 * Hall Laboratory
 * Quinlan Laboratory
 * Department of Computer Science
 * Department of Biochemistry and Molecular Genetics
 * Department of Public Health Sciences and Center for Public Health Genomics,
 * University of Virginia
 * rl6sf@virginia.edu
 *
 * Licenced under the GNU General Public License 2.0 license.
 * ***************************************************************************/

#include "SV_PairReader.h"
#include "SV_Pair.h"
#include "SV_Tools.h"
#include "ucsc_bins.hpp"
#include <iostream>

//{{{ SV_PairReader:: SV_PairReader()
SV_PairReader::
SV_PairReader(struct pair_end_parameters pair_end_param)
{
	is_open = false;
	bam_file = pair_end_param.bam_file;
	histo_file = pair_end_param.histo_file;
	mean = pair_end_param.mean;
	stdev = pair_end_param.stdev;
	read_length = pair_end_param.read_length;
	min_non_overlap = pair_end_param.min_non_overlap;
	discordant_z = pair_end_param.discordant_z;
	back_distance = pair_end_param.back_distance;
	weight = pair_end_param.weight;
	id = pair_end_param.id;
	min_mapping_threshold = pair_end_param.min_mapping_threshold;
}
//}}}

//{{{ SV_PairReader:: SV_PairReader()
SV_PairReader::
SV_PairReader()
{
	bam_file = "";
	histo_file = "";
	mean = 0;
	stdev = 0;
	read_length = 0;
	min_non_overlap = 0;
	discordant_z = 0;
	back_distance = 0;
	weight = 0;
	id = -1;
	min_mapping_threshold = 0;
	sample_id = SV_EvidenceReader::counter;
	SV_EvidenceReader::counter = SV_EvidenceReader::counter + 1;

	inited = false;
}
//}}}

//{{{ string SV_PairReader:: check_params()
string
SV_PairReader::
check_params()
{
	string msg = "";

	if (bam_file.compare("") == 0)
		msg.append("bam_file ");
	if (histo_file.compare("") == 0)
		msg.append("histo_file ");
	if (mean == 0)
		msg.append("mean ");
	if (stdev == 0)
		msg.append("stdev ");
	if (read_length == 0)
		msg.append("read_length ");
	if (min_non_overlap == 0)
		msg.append("min_non_overlap ");
	if (discordant_z == 0)
		msg.append("discordant_z ");
	if (back_distance == 0)
		msg.append("back_distance ");
	if (weight == 0)
		msg.append("weight ");
	if (id == -1)
		msg.append("id ");

	return msg;
}
//}}}

//{{{ bool SV_PairReader::: add_param(string param, string val)
bool
SV_PairReader::
add_param(char *param, char *val)
{
	if ( strcmp("bam_file", param) == 0 )
		bam_file = val;
	else if ( strcmp("histo_file", param) == 0 )
		histo_file = val;
	else if ( strcmp("mean", param) == 0 )
		mean = atof(val);
	else if ( strcmp("stdev", param) == 0 )
		stdev = atof(val);
	else if ( strcmp("read_length", param) == 0 )
		read_length = atoi(val);
	else if ( strcmp("min_non_overlap", param) == 0 )
		min_non_overlap = atoi(val);
	else if ( strcmp("discordant_z", param) == 0 )
		discordant_z = atoi(val);
	else if ( strcmp("back_distance", param) == 0 )
		back_distance = atoi(val);
	else if ( strcmp("weight", param) == 0 )
		weight = atoi(val);
	else if ( strcmp("id", param) == 0 )
		id = atoi(val);
	else if ( strcmp("min_mapping_threshold", param) == 0 )
		min_mapping_threshold = atoi(val);
	else 
		return false;

	return true;
}
//}}}

//{{{ struct pair_end_parameters SV_PairReader:: get_pair_end_parameters()
struct pair_end_parameters 
SV_PairReader::
get_pair_end_parameters()
{
	struct pair_end_parameters pair_end_param;

	pair_end_param.bam_file = bam_file;
	pair_end_param.histo_file = histo_file;
	pair_end_param.mean = mean;
	pair_end_param.stdev = stdev;
	pair_end_param.read_length = read_length;
	pair_end_param.min_non_overlap = min_non_overlap;
	pair_end_param.discordant_z = discordant_z;
	pair_end_param.back_distance = back_distance;
	pair_end_param.weight = weight;
	pair_end_param.id = id;
	pair_end_param.min_mapping_threshold = min_mapping_threshold;

	return pair_end_param;
}
//}}}

//{{{ void SV_PairReader:: set_statics()
void
SV_PairReader::
set_statics()
{
	SV_Pair::min_mapping_threshold = min_mapping_threshold;
	SV_Pair::min_non_overlap = min_non_overlap;
	SV_Pair::insert_mean = mean;
	SV_Pair::insert_stdev = stdev;
	SV_Pair::insert_Z = discordant_z;
	SV_Pair::back_distance = back_distance;
	SV_Pair::read_length = read_length;

	int distro_size;
	unsigned int start, end;

	SV_Pair::histo_size = read_histo_file(histo_file, 
										   &(SV_Pair::histo),
										   &start,
										   &end);

	SV_Pair::histo_start = start;
	SV_Pair::histo_end = end;

	SV_Pair::set_distro_from_histo();
}
//}}}

//{{{ void SV_PairReader:: initialize()
void
SV_PairReader::
initialize()
{
	//cerr << "Pair initialize" << endl;
	// open the BAM file
	reader.Open(bam_file);

	// get header & reference information
	header = reader.GetHeaderText();
	refs = reader.GetReferenceData();

	have_next_alignment = reader.GetNextAlignment(bam);
	//cerr << "Pair have_next_alignment " << have_next_alignment << endl;
}
//}}}

//{{{ void SV_PairReader:: process_input()
void
SV_PairReader::
process_input( UCSCBins<SV_BreakPoint*> &r_bin)
{
	while (reader.GetNextAlignment(bam)) 
		if (bam.IsMapped() && bam.IsMateMapped())  //Paired read
			SV_Pair::process_pair(bam,
								  refs,
								  mapped_pairs,
								  r_bin,
								  weight,
								  id,
								  sample_id);
}
//}}}

//{{{ string SV_PairReader:: get_curr_chr()
string
SV_PairReader::
get_curr_chr()
{
	//cerr << "Pair get_curr_chr" << endl;
	return refs.at(bam.RefID).RefName;
}
//}}}

//{{{ void SV_PairReader:: process_input_chr(string chr,
void
SV_PairReader::
process_input_chr(string chr,
				  UCSCBins<SV_BreakPoint*> &r_bin)
{
	// Process this chr, or the next chr 
	while ( have_next_alignment &&
			( chr.compare( refs.at(bam.RefID).RefName) == 0 ) ) {
		if (bam.IsMapped() && bam.IsMateMapped())  //Paired read
			SV_Pair::process_pair(bam,
								  refs,
								  mapped_pairs,
								  r_bin,
								  weight,
								  id,
								  sample_id);
		have_next_alignment = reader.GetNextAlignment(bam);
		if ( bam.RefID < 0 )
			have_next_alignment = false;
	}
}
//}}}

//{{{ void SV_PairReader:: terminate()
void 
SV_PairReader::
terminate()
{
	//cerr << "SplitRead Done" << endl;
	reader.Close();
}
//}}}

//{{{ bool SV_EvidenceReader:: has_next()
bool
SV_PairReader::
has_next()
{
	//cerr << "Pair has_next:" << have_next_alignment << endl;
	return have_next_alignment;
}
//}}}
