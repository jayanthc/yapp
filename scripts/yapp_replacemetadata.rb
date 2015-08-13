#!/usr/bin/ruby

#
# yapp_replacemetadata.rb
# Replaces the header in a SIGPROC filterbank file with information from a
# YAPP metadata (.ym) file.
#
# Created by Jayanth Chennamangalam on 2015.07.08
#

require 'getoptlong'


def printUsage(progName)
  puts <<-EOF
Usage: #{progName} [options] <data-file>
    -h  --help                          Display this usage information
    -m  --metadata <file>               Metadata file
  EOF
end


opts = GetoptLong.new(
  [ '--help',     '-h', GetoptLong::NO_ARGUMENT ],
  [ '--metadata', '-m', GetoptLong::REQUIRED_ARGUMENT ],
)

# declare variables
fileDataTempName = "yapptempdata"
fileDataTemp = nil
fileYM = nil
fileYMB = nil

opts.each do |opt, arg|
  case opt
    when '--help'
      printUsage($PROGRAM_NAME)
      exit
    when '--metadata'
      fileYM = arg
  end
end

# user input validation
if nil == fileYM
  STDERR.puts "ERROR: Metadata file not specified!"
  printUsage($PROGRAM_NAME)
  exit
end

if File.extname(fileYM) != ".ym"
  STDERR.puts "ERROR: Unrecognized file extension! Not \".ym\"!"
  printUsage($PROGRAM_NAME)
  exit
end

fileData = ARGV[0]

if File.extname(fileData) != ".fil"
  STDERR.puts "ERROR: Unrecognized file extension! Not \".fil\"!"
  printUsage($PROGRAM_NAME)
  exit
end

# convert metadata to binary and write it out to the metadata file
%x[yapp_ym2ymb #{fileYM}]

# build metadata file name
fileYMB = File.dirname(fileYM) + "/" + File.basename(fileYM, ".ym") + ".ymb"

# read data file header size
lenHeader = (%x[yapp_viewmetadata #{fileData} | tail -1 | cut -d ":" -f 2 | sed -e "s/^[ ]//"]).to_i

# build temporary data file path
fileDataTemp = File.dirname(fileData) + "/" + fileDataTempName

# copy data from input to temporary file
%x[dd if=#{fileData} of=#{fileDataTemp} ibs=#{lenHeader} obs=104857600 skip=1]

# replace old data with new metadata
%x[cat #{fileYMB} > #{fileData}]

# concatenate temporary data file to old data file
%x[cat #{fileDataTemp} >> #{fileData}]

# remove temporary files
%x[rm #{fileYMB}]
%x[rm #{fileDataTemp}]

