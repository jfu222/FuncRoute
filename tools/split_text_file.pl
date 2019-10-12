#!/usr/bin/perl -w
use strict;
use File::Basename;

my $help=<<qq;
Usage:
   perl $0 <in|FuncRoute_pdf.tex> <in|num_of_pdf_pages_per_tex_after_split> <out_dir|FuncRoute_pdf_split_dir>
For Example:
   perl $0 ./FuncRoute_pdf.tex 100 ./FuncRoute_pdf_split_dir
qq

die "$help" unless @ARGV == 3;

my ($file_tex, $num_of_pdf_pages_per_tex_after_split, $out_dir) = @ARGV[0, 1, 2];

$file_tex =~ s/\\/\//g;
$out_dir =~ s/\\/\//g;

print "out_dir: $out_dir\n";

if(!-d "$out_dir")
{
	`mkdir -p "$out_dir"`;
}

#---------------------------
my $file_basename = basename($file_tex, qw(.tex));

my $tex_header = "";
my $flag = 0;
my $file_cnt = 1;
my $page_cnt = 0;
my $out_file_cnt = "";
my @out_files = ();

open IN, "< $file_tex" or die $!;
while(<IN>)
{
	if(/begin\{tikzpicture\}/)
	{
		if($flag == 0)
		{
			$out_file_cnt = "$out_dir/$file_basename.$file_cnt.tex";
			print "$out_file_cnt\n";
			push @out_files, "$out_file_cnt";
			open OT, "> $out_file_cnt" or die $!;
			print OT "$tex_header";
			$file_cnt++;
			$flag = 1;
		}
		
		print OT "$_";
		
	}
	elsif(/end\{tikzpicture\}/)
	{
		$page_cnt++;
		print OT "$_";
		
		if($page_cnt % $num_of_pdf_pages_per_tex_after_split == 0)
		{
			print OT "\\end{document}\n";
			close OT;
			
			#-----------------------
			$out_file_cnt = "$out_dir/$file_basename.$file_cnt.tex";
			print "$out_file_cnt\n";
			push @out_files, "$out_file_cnt";
			open OT, "> $out_file_cnt" or die $!;
			print OT "$tex_header";
			$file_cnt++;
			$page_cnt = 0;
		}
	}
	elsif(/end\{document\}/)
	{
		print OT "$_";
		close OT;
	}
	else
	{
		if($flag == 0)
		{
			$tex_header .= $_;
		}else
		{
			print OT "$_";
		}
	}
}
close IN;

#-------------------------
foreach my $i(0..$#out_files)
{
	my $cmd = "pdflatex -output-directory=$out_dir $out_files[$i]";
	print "$i/$#out_files: $cmd\n";
	`$cmd`;
}

#-------------------------
print "All thing is OK!\n";
