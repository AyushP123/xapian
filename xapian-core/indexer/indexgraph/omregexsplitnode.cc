/* omregexsplitnode.cc: Implementation of the regex split node
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002 Ananova Ltd
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#include <config.h>
#include "regexcommon.h"
#include "om/omindexernode.h"
#include "node_reg.h"

/** Node which splits a string into separate strings with a regular
 *  expression separator.
 *
 *  The omregexsplit node takes a string and splits it into fields
 *  with a separator specified as a regular expression.  A typical
 *  use of this might be to split a string on words separated by
 *  whitespace, or comma-separated fields.
 *
 *  Example: input string 'foo bar, wibble,  wobble' with regular
 *  expression ', *' would return ["foo bar", "wibble", "wobble"].
 *
 *  Inputs:
 *  	in: The input string 
 *  	regex: The regular expression to use, in POSIX syntax.  Will be
 *  		ignored if the parameter is specified.
 *
 *  Outputs:
 *  	output: The list of strings between matches of the regular
 *  		expression.
 *
 *  Parameters:
 *  	regex: The regular expression used for matching.  The syntax is
 *  		the standard POSIX regular expression syntax.  This
 *  		parameter, if specified, causes the regex input to be
 *  		ignored.
 */
class OmRegexSplitNode : public OmIndexerNode {
    public:
	OmRegexSplitNode(const OmSettings &config)
		: OmIndexerNode(config),
		  config_regex(config.get("regex", "")),
		  regex_from_config(config_regex.length() > 0)
	{
	    if (regex_from_config) {
		regex.set(config_regex);
	    }
	}
    private:
	std::string config_regex;
	bool regex_from_config;
	Regex regex;
	void config_modified(const std::string &key)
	{
	    if (key == "regex") {
		config_regex = get_config_string(key);
		regex_from_config = config_regex.length() > 0;
		if (regex_from_config) {
		    regex.set(config_regex);
		}
	    }
	}
	void calculate() {
	    request_inputs();
	    OmIndexerMessage mess = get_input_record("in");
	    if (mess.get_type() == OmIndexerMessage::rt_empty) {
		set_empty_output("out");
		return;
	    }
	    std::string input = mess.get_string();

	    if (!regex_from_config) {
		regex.set(get_input_string("regex"));
	    }

	    OmIndexerMessage output;
	    output.set_vector();

	    /* There's a problem if we have an expression which can
	     * match a null string (eg " *") - we never get anywhere
	     * along the source string.  The workaround for this is
	     * to advance a character and try again.  Then we add the
	     * character we stepped over to the next word sent to the
	     * output.
	     */
	    int extra_char = -1;

	    // current position in input string
	    std::string::size_type pos = 0;
	    std::string::size_type last = input.length();
	    while (pos < last && regex.matches(input, pos)) {
		size_t start = regex.match_start(0);
		size_t end = regex.match_end(0);

		if (start > pos) {
		    if (extra_char == -1) {
			output.append_element(input.substr(pos, start - pos));
		    } else {
			std::string temp;
			temp += (unsigned char)extra_char;
			temp += input.substr(pos, start - pos);
			output.append_element(temp);
			extra_char = -1;
		    }
		}
		if (end == pos) {
		    if (extra_char != -1) {
			std::string temp;
			temp += (unsigned char)extra_char;
			output.append_element(temp);
			extra_char = -1;
		    } else {
			extra_char = (unsigned char)input[pos];
			pos++;
		    }
		} else if (end < last) {
		    pos = end;
		} else {
		    pos = last;
		}
	    }
	    if (pos < last || extra_char != -1) {
		std::string last_one;
		if (extra_char != -1) {
		    last_one += extra_char;
		}
		last_one += input.substr(pos);
		output.append_element(last_one);
	    }
	    set_output("out", output);
	}
};

NODE_BEGIN(OmRegexSplitNode, omregexsplit)
NODE_INPUT("in", "string", mt_string)
NODE_INPUT("regex", "string", mt_string)
NODE_OUTPUT("out", "strings", mt_vector)
NODE_END()
