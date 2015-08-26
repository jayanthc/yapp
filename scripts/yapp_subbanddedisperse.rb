#!/usr/bin/ruby

#
# yapp_subbanddedisperse.rb
# Runs yapp_dedisperse multiple times with appropriate options to create a set
# of sub-band-dedispersed time series, then runs yapp_stacktim to create a
# sub-band-dedispersed filterbank data file. Optionally does smoothing.
#
# Created by Jayanth Chennamangalam on 2015.06.13
#

require 'getoptlong'


def printUsage(progName)
  puts <<-EOF
Usage: #{progName} [options]
    -h  --help                          Display this usage information
    -d  --dm <dm>                       DM at which to de-disperse
    -b  --nsubband <nsubband>           Number of sub-bands
                                        (must be < number of channels)
    -w  --width <width>                 Width of boxcar window in milliseconds
  EOF
end


opts = GetoptLong.new(
  [ '--help',     '-h', GetoptLong::NO_ARGUMENT ],
  [ '--dm',       '-d', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--nsubband', '-b', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--width',    '-w', GetoptLong::REQUIRED_ARGUMENT ],
)

# declare variables
dm = 0.0                # DM
numSubBand = 0          # number of sub-bands
startSubBand = 0        # minimum value in the sub-band range
stopSubBand = 0         # maximum value in the sub-band range
width = 0.0             # smoothing window duration
numUserInput = 2        # expect at least DM and number of sub-bands
userInputGiven = 0      # flag to check whether user input is given
extName = nil
baseName = nil

opts.each do |opt, arg|
  case opt
    when '--help'
      printUsage($PROGRAM_NAME)
      exit
    when '--dm'
      dm = arg.to_f
      userInputGiven += 1
    when '--nsubband'
      numSubBand = arg.to_i
      userInputGiven += 1
    when '--width'
      width = arg.to_f
  end
end

# user input validation
if userInputGiven < 2
  STDERR.puts "ERROR: Missing user input!"
  printUsage($PROGRAM_NAME)
  exit
end

if numSubBand <= 0
  STDERR.puts "ERROR: Incorrect user input! Number of sub-bands must be > 0!"
  printUsage($PROGRAM_NAME)
  exit
end

file = ARGV[0]

# perform dedispersion for the given number of sub-bands
for i in 0...numSubBand
    %x[yapp_dedisperse -d #{dm} -b #{numSubBand} -u #{i} #{file}]
end

# build the output file names for the preceding step and stack those .tim files
# together to form a sub-band-dedispersed filterbank data file
extName = File.extname(file)
if extName != ".fil" and extName != ".spec"
  STDERR.puts "ERROR: Unrecognized file extension! Not \".fil\" or \".spec\"!"
  printUsage($PROGRAM_NAME)
  exit
end
baseName = File.basename(file, extName)
inputFileGlob = baseName + ".dm" + sprintf("%g", dm) + ".band*.tim"

# perform optional smoothing
if width != 0.0
  %x[ls #{inputFileGlob} | xargs -n 1 yapp_smooth -w #{width}]

  # update inputFileGlob for stacking
  inputFileGlob = baseName + ".dm" + sprintf("%g", dm) + ".band*.smooth"      \
                  + sprintf("%g", width) + ".tim"
end

%x[yapp_stacktim -e #{inputFileGlob}]

# delete intermediate files
# delete sub-band time series files
inputFileGlob = baseName + ".dm" + sprintf("%g", dm) + ".band?.tim"
%x[rm -f #{inputFileGlob}]
inputFileGlob = baseName + ".dm" + sprintf("%g", dm) + ".band??.tim"
%x[rm -f #{inputFileGlob}]
# delete smoothed time series files
inputFileGlob = baseName + ".dm" + sprintf("%g", dm) + ".band*.smooth*.tim"
%x[rm -f #{inputFileGlob}]

