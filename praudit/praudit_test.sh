#
# Copyright (c) 2018 Aniket Pandey
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD$
#


atf_test_case praudit
praudit_body()
{
	# Check that comma delimiter is present with -d "," cmd
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/del_comma \
			praudit -d "," $(atf_get_srcdir)/trail

	# Check that underscore delimiter is present with -d "_" cmd
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/del_underscore \
			praudit -d "_" $(atf_get_srcdir)/trail

	# Check that praudit outputs default form without arguments
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/no_args \
			praudit $(atf_get_srcdir)/trail

	# Check that praudit outputs the numeric form with "-n" flag
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/numeric_form \
			praudit -n $(atf_get_srcdir)/trail

	# Check that praudit outputs the raw form with "-r" flag
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/raw_form \
			praudit -r $(atf_get_srcdir)/trail

	# Check that praudit outputs the trail in same line with "-l" flag
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/same_line \
			praudit -l $(atf_get_srcdir)/trail

	# Check that praudit outputs the short form with "-s" flag
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/short_form \
			praudit -s $(atf_get_srcdir)/trail

	# Check that praudit outputs the XML file with "-x" flag
	atf_check -s exit:1 -o file:$(atf_get_srcdir)/xml_form \
			praudit -x $(atf_get_srcdir)/trail
}

atf_init_test_cases()
{
	atf_add_test_case praudit
}