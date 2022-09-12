eval 'exec perl -w -S $0 ${1+"$@"}'
if 0; # not running under some shell

use strict;
use warnings;
use Time::Piece;

# Rain gauge overflow value
my $RAINGAUGE_MAX_VALUE = 100;

my $file = $ARGV[0];		# input file
my $hourly  = 0;
my $daily   = 0;
my $weekly  = 0;
my $monthly = 0;
my $prevDay = -1;
my $prevHour;
my $prevWeek;
my $prevMonth;
my $rain_acc = 0;
my @hour = ();
my $cnt = 0;
my $no_lines = 5000;
usage() unless $#ARGV >= 0;

die "Error: can't read $file.\n" if (!-r $file); # check if file is readable
open (INFO, "<$file") || die "Can't open $file.\n";

# skip first line
$_ = <INFO>;

# read entire file to string
my $line;
foreach $line (<INFO>) {			# read line by line
  $cnt = $cnt + 1;
  my ($ts, $rain) = split(",", $line);
  my $dt = Time::Piece->strptime($ts, '%d/%m/%Y %H:%M');
  
  push @hour, $rain;
  
  if (@hour > 4) {
    $_ = shift(@hour);
  }
  $hourly = 0;
  my $i;
  foreach $i (@hour) {
     $hourly = $hourly + $i;
  }
  
  if ($prevDay == -1) {
    $prevDay   = $dt->wday;
    $prevWeek  = $dt->week;
    $prevMonth = $dt->mon;
    $daily  = $rain;
    $weekly = $rain;
    $monthly = $rain;
  } else {
    if ($dt->wday != $prevDay) {
      $daily = 0;
    } else {
      $daily = $daily + $rain;
    }
  
    if ($dt->week != $prevWeek) {
      $weekly = 0;
    } else {
      $weekly = $weekly + $rain;
    }

    if ($dt->mon != $prevMonth) {
      $monthly = 0;
    } else {
      $monthly = $monthly + $rain;
    }

    $prevDay   = $dt->wday;
    $prevWeek  = $dt->week;
    $prevMonth = $dt->mon;
  }
  $rain_acc = $rain_acc + $rain;
  if ($rain_acc >= $RAINGAUGE_MAX_VALUE) {
    $rain_acc = $rain_acc - $RAINGAUGE_MAX_VALUE;
  }
  
  #print $dt->strftime('%F %T') . "H: $hourly  D: $daily  W: $weekly  M: $monthly\n";
  #print $dt->strftime('%F %T') . ",$rain,$hourly,$daily,$weekly,$monthly\n";
  my $timestr = $dt->strftime('%F %H:%M');
  printf(   "  // $timestr -> $rain; H: $hourly; D: $daily; W: $weekly; M: $monthly\n");
  printf( qq{  setTime("$timestr", tm, ts);\n} );
  printf( qq{  rainGauge.update(tm, $rain_acc);\n} );
  printf( qq{  DEBUG_CB();\n});
  printf( qq{  DOUBLES_EQUAL(%7.1f, rainGauge.pastHour(),     TOLERANCE);\n}, $hourly);
  printf( qq{  DOUBLES_EQUAL(%7.1f, rainGauge.currentDay(),   TOLERANCE);\n}, $daily);
  printf( qq{  DOUBLES_EQUAL(%7.1f, rainGauge.currentWeek(),  TOLERANCE);\n}, $weekly);
  printf( qq{  DOUBLES_EQUAL(%7.1f, rainGauge.currentMonth(), TOLERANCE);\n}, $monthly);
  print "\n";
  
  if ($cnt == $no_lines) {
    last;
  }
}
print "}\n";
close INFO;

sub usage {
    my $script_name = `basename $0`;
    
    chop $script_name;
    
    printf("\n    SYNTAX : %s %s\n", $script_name, "<csv_file>");
  
  print <<END_OF_HELP;

    PROGRAM DESCRIPTION:
      This Perl script generates unit test data for RainGauge from rain data in CSV file.
      
      Expected CSV file format: 
      DateTime, mm
      12/06/13 00:00,0
      12/06/13 00:15,0.4
      [...]
      
      DateTime: d/m/y H:M
      The result is printed to STDOUT.

END_OF_HELP

  exit;
}
