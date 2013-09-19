#!/usr/bin/ruby

#
# yapp_viewcand.rb
# Script to read all PNG images in the specified directory and create a set of
#   HTML pages, one for each DM, that show tiled images of candidate plots.
#   This script assumes that file names are of the form
#   <prefix><dm><infix><p><postfix>, where <dm> is the DM and <p> is the
#   period. For example,
#   p2557.20100715.NGC6838.b6s1g0.8bit_DM117.11.sum_4.89ms_Cand.pfd.ps.
#
# Created by Jayanth Chennamangalam on 2013.09.18
#

require 'getoptlong'


def printUsage(progName)
  puts <<-EOF
Usage: #{progName} [options]
    -h  --help                          Display this usage information
    -m  --mogrify                       Mogrify PS files to PNG first
    -d  --dmstart                       First DM in range
    -t  --dmstep                        DM step size
    -s  --dmstop                        Last DM in range
    -p  --prefix                        Path and prefix of PS files
    -i  --infix                         Infix between DM and P, of PS files
    -x  --postfix                       Postfix after P, of PS files
                                        (default is "ms_Cand.pfd.ps")
    -r  --rows                          Number of rows in the plot table
                                        (default is 2)
    -c  --columns                       Number of columns in the plot table
                                        (default is 4)
  EOF
end


opts = GetoptLong.new(
  [ '--help',    '-h', GetoptLong::NO_ARGUMENT ],
  [ '--mogrify', '-m', GetoptLong::NO_ARGUMENT ],
  [ '--dmstart', '-d', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--dmstep',  '-t', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--dmstop',  '-s', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--prefix',  '-p', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--infix',   '-i', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--postfix', '-x', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--rows',    '-r', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--columns', '-c', GetoptLong::REQUIRED_ARGUMENT ]
)

# declare variables
# default values for optional input
numRows = 2
numCols = 4
postfix = "ms_Cand.pfd.ps"      # immediately follows the period
# initialise values to 0 for required input
startDM = 0.0       # minimum value in the DM range
stepDM = 0.0        # DM step size
stopDM = 0.0        # maximum value in the DM range
prefix = nil        # prefix, e.g., "p2557.20100715.Pal10.b6s1g0.8bit_DM",
                    # which immediately precedes the DM value
infix = nil         # infix, e.g., "0.sum_", which immediately follows the DM
                    # value and immediately precedes the period value

opts.each do |opt, arg|
  case opt
    when '--help'
      printUsage($PROGRAM_NAME)
      exit
    when '--mogrify'
      # mogrify all PS files to PNG
      print "Mogrifying plots..."
      STDOUT.flush
      %x[mogrify -rotate 90 -geometry 300 -format png #{prefix}*.ps]
      puts "DONE"
    when '--dmstart'
      startDM = arg.to_f
    when '--dmstep'
      stepDM = arg.to_f
    when '--dmstop'
      stopDM = arg.to_f
    when '--prefix'
      prefix = arg
    when '--infix'
      infix = arg
    when '--postfix'
      postfix = arg
    when '--rows'
      numRows = arg.to_i
    when '--columns'
      numCols = arg.to_i
  end
end

# user input validation
if 0.0 == stepDM or 0.0 == stopDM or nil == prefix
  STDERR.puts "ERROR: Missing user input!"
  printUsage($PROGRAM_NAME)
  exit
end

if 0 == numRows or 0 == numCols
  STDERR.puts "ERROR: Invalid user input!"
  printUsage($PROGRAM_NAME)
  exit
end

if 1 == numCols
  STDERR.puts "ERROR: Number of columns must be at least 2!"
  printUsage($PROGRAM_NAME)
  exit
end

files = Dir.glob(prefix + "*ms_Cand.pfd.png")
if 0 == files.length
  STDERR.puts "ERROR: No PNG file found! Run with the --mogrify/-m option."
  printUsage($PROGRAM_NAME)
  exit
end

print "Generating HTML pages..."
STDOUT.flush

# change postfix extension
postfix = File.basename(postfix, ".ps") + ".png"

DM = (startDM..stopDM).step(stepDM).to_a
numDMs = DM.length

# create CSS file
str = "body { font-family: \"Tahoma\", sans-serif; font-size: 10pt; }\n"
str << "td { border-style: solid; border-width: 1px; padding: 1px; "
str << "width: 300px; }\n"
str << "td.headfoot { border-width: 0px; }\n"
str << "td.dummy { border-width: 0px; }\n"
str << "td#prevNext { text-align: right; }\n"
str << "td#footer { text-align: right; font-size: 8pt; }\n"
str << "a { color: #003333; } "
str << "a:hover { color: #E34C26; }\n"
str << "a:active { color: #E34C26; }\n"
fileCSS = File.new("#{prefix}.css", "w")
fileCSS << str
fileCSS.close

startPage = "#{prefix}#{DM[0]}_0.htm"
prevPage = nil
for i in 0...numDMs
  files = Dir.glob(prefix + "#{DM[i]}0.sum_*ms_Cand.pfd.png")
  numFiles = files.length
  k = 0
  row = 0
  col = 0
  for j in 0...numFiles
    if 0 == row and 0 == col
      str = "<!doctype html>\n<html>\n<head>\n"
      str << "<meta http-equiv=\"Content-Type\" "
      str << "content=\"text/html;charset=utf-8\" />\n"
      str << "<link rel=\"stylesheet\" type=\"text/css\" "
      str << "href=\"#{prefix}.css\" />\n"
      str << "<title>yapp_viewcand</title>\n"
      str << "</head>\n<body>\n"
      numPages = (Float(numFiles) / (numCols * numRows)).ceil
      str << "<table>\n<tr>\n"
      str << "<td class=\"headfoot\" colspan=\"#{numCols-1}\">DM = #{DM[i]} "
      str << "(#{i+1} / #{numDMs}), "
      str << "# Candidates = #{numFiles}</td>"
      str << "<td class=\"headfoot\" id=\"prevNext\">"
      str << "Page = #{k+1} / #{numPages}&nbsp;"
      if nil == prevPage
        str << "Start&nbsp;"
        str << "Prev"
        nextPage = "#{prefix}#{DM[i]}_#{k+1}.htm"
      else
        str << "<a href=\"#{startPage}\">Start</a>&nbsp;"
        str << "<a href=\"#{prevPage}\">Prev</a>"
      end
      if nil == nextPage
        str << "&nbsp;Next"
      else
        if k < numPages - 1
          nextPage = "#{prefix}#{DM[i]}_#{k+1}.htm"
          str << "&nbsp;<a href=\"#{nextPage}\">Next</a>"
        else
          if i < numDMs - 1
            nextPage = "#{prefix}#{DM[i+1]}_0.htm"
            str << "&nbsp;<a href=\"#{nextPage}\">Next</a>"
          else
            nextPage = nil
            str << "&nbsp;Next"
          end
        end
      end
      str << "</td></tr>\n<tr>\n"
    end
    p = files[j].match(/#{prefix}#{DM[i]}#{infix}(.*)#{postfix}/)[1]
    str << "<td>P = #{p} ms<br /><img src=\"#{files[j]}\" "
    str << "alt=\"P = #{p}\" /></td>\n"
    col = col + 1
    if numCols == col
      str << "</tr>\n<tr>\n"
      row = row + 1
      if numRows == row
        str << "<td class=\"headfoot\" id=\"footer\" colspan=\"#{numCols}\">"
        str << "Page generated by "
        str << "<a href=\"http://jayanthc.github.io/yapp/yapp_viewcand.htm\">"
        str << "yapp_viewcand.rb"
        str << "</a>"
        str << "</td></tr>\n"
        str << "</table>\n&nbsp;<br />&nbsp;<br />\n"
        str << "</body>\n</html>\n"

        fileHTML = File.new("#{prefix}#{DM[i]}_#{k}.htm", "w")
        fileHTML << str
        fileHTML.close

        prevPage = "#{prefix}#{DM[i]}_#{k}.htm"
        k = k + 1
        row = 0
      end
      col = 0
    end
  end
  if numPages - 1 == k
    # create dummy columns
    for l in 0...(numCols - col)
      str << "<td class=\"dummy\">&nbsp;</td>\n"
    end
    # add footer
    str << "<tr>"
    str << "<td class=\"headfoot\" id=\"footer\" colspan=\"#{numCols}\">"
    str << "Page generated by "
    str << "<a href=\"http://jayanthc.github.io/yapp/yapp_viewcand.htm\">"
    str << "yapp_viewcand.rb"
    str << "</a>"
    str << "</table>\n&nbsp;<br />&nbsp;<br />\n"
    str << "</body>\n</html>\n"

    fileHTML = File.new("#{prefix}#{DM[i]}_#{k}.htm", "w")
    fileHTML << str
    fileHTML.close

    prevPage = "#{prefix}#{DM[i]}_#{k}.htm"
  end
end

puts "DONE\n"

