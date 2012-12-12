#!/usr/bin/perl -w

use strict;

use DBI;
use JSON;


my ( $req, $arg, $ret );
my %ora;
	# Read input parameters from AJAX ##############
while(<STDIN>) {
 chomp;
        if($. == 1) {
                /^(\w*)$/;
                $req = $1;
        } else {
                $arg .= $_;
        }
}

print <<EOF;
Content-Type: text/plain; charset=utf-8
Cache-Control: private

EOF

	# Exec request and return JSON string #########

my %subr = (
        cmdSelect        => \&selectData,
	cmdModify	 => \&modifyData,
    );

$ret = main($req, from_json($arg, {utf8 => 1}));
print to_json($ret, {utf8 => 1, pretty => 1});

#############################################################
#	Main procedure
#############################################################
sub main{
    my $cmd = $_[0];
    my %arg = %{ $_[1] };   #input hash
    my $err;

    errRet("Command is not supported", '', \%arg) unless exists $subr{$cmd};

        # connect to database ################

    $err = readIni('fts3config.ini');   #Read ini file for Oracle
    errRet($err, '',\%arg) if $err; #exit if error;

    $err=0;
    my $dbh = DBI->connect("dbi:$ora{'connstring'}",$ora{'user'},$ora{'pass'}, {PrintWarn => 0, PrintError => 0, RaiseError => 0 })
                  or $err = $DBI::errstr;
    errRet( 'Error database connection',$err, \%arg ) if $err;

    return $subr{$cmd}( $dbh, \%arg );
}

#############################################################
#       Select required data from database
#Input:
# {
#    cmd => 'selectData',
#    query => < select query >,
#    vals => [ array of binding values ], #optional
#
#Output:
# {
#    status => < 0/1 >,    # 0=OK
#    retMsg => < error message
#    retNative => error from DBI if present
#    rowNum => number of rows
#    data => [
#		{ colname1 => value1, colname2 => value2, ....},
#		{ colname1 => value1, colname2 => value2, ....},
#	     ]
# }

#############################################################
sub selectData{
    my ( $dbh, $arg ) = @_;
    my $err;

    $dbh->{FetchHashKeyName} = 'NAME_lc';

	# prepare sql select request and execute ############

    my @sql_bind;
    my $qr = $arg->{query};   #select query 
    if( exists $arg->{vals} ){ 	#exists bind values
	@sql_bind = @{$arg->{vals}};
    }

    my $sth = $dbh->prepare($qr) or errRet('Error when select query', $dbh->errstr, $arg);
    
    if( exists $arg->{vals} ){
	$sth->execute(@sql_bind) or $err = $sth->errstr;
    }else{
        $sth->execute or $err = $sth->errstr;
    }
    errRet('Error when select query', $err, $arg) if $err;

	# make output array of hashes ###############

    my $ind = 0;
#    $arg->{data} = ();
    while( my $href = $sth->fetchrow_hashref ){
#	push @arr, $href;	#not recomended!?
	foreach my $key ( keys %{$href} ){
	    $arg->{data}[$ind]->{$key} = $href->{$key};
	}
	$ind++
    }
    $arg->{rowNum} = $sth->rows;
    $arg->{status} = 0;

    $sth->finish;
    $dbh->disconnect;
    return $arg;
}
#####################################################################
#	Modify tables
#Input:
# {
#    cmd => 'cmdModify',
#    cmdPack => array of hash
#	[
#	  {
#	    cmd => <insert,delete,update>
#	    table => <table name>
#	    cols => [columns names ],
#	    vals => [values],
#	    keyNames => [ key names ],    for delete command
#	    keyValues => [ key values ]     and update
#	  },
#	  {...},
#	]
# }
#Output:
# {
#    status => < 0/1 >, # =0 if OK
#    retMsg => < error message >
#    retNative => error from DBI if present
# }
#####################################################################
sub modifyData{
    my ( $dbh, $arg ) = @_;
    my ( $err, $qr, $where, $bind );
    my @arval;
    my @whereval;

    $dbh->{AutoCommit} = 0;
    for my $hr ( @{$arg->{cmdPack}} ){  #ref to hash
	@arval = ();
	@whereval = ();
	$where = '';

	# create where clause if need

	if( $hr->{cmd} eq 'delete' or $hr->{cmd} eq 'update' ){
	    errRet("Input error: Cmd=$hr->{cmd},Table=$hr->{table},  undefined keyNames and keyValues", '', $arg) unless exists $hr->{keyNames} and exists $hr->{keyValues};

	    push @whereval, @{$hr->{keyValues}};
	    my $ind = @{$hr->{keyNames}}-1;
	    for my $i ( 0..$ind ){
		$where .= qq{$hr->{keyNames}[$i] = ? };
		$where .= 'and ' unless $i == $ind;
	    }
	}

	# create sql request
	if( $hr->{cmd} eq 'insert' ){				#insert command
            #check and remove undef fields
            my @names;
            my @vals;
            my $n = @{$hr->{cols}};
            for my $i (0..$n-1){
                next if $hr->{vals}[$i] eq '';  #skip undef columns
                push @names, $hr->{cols}[$i];
                push @vals, $hr->{vals}[$i];
            }

	    $bind = join ", ", ("?") x @names;
	    $qr = "insert into $hr->{table} ("    
            . join(", ", @names)        
            . ") values( $bind )";
	    @arval = @vals;
	}
	elsif( $hr->{cmd} eq 'delete' ){			#delete command
	    $qr = "delete from $hr->{table} where $where ";
	    @arval = @whereval;
	}
	elsif( $hr->{cmd} eq 'update' ){			#update command
	    my $set = '';
	    my $ind = @{$hr->{cols}}-1;
	    for my $i ( 0..$ind ){
		$set .= qq{$hr->{cols}[$i] = ?,};
	    }
	    chop $set;
	    $qr = qq{
		update $hr->{table}
		set $set
		where $where
	    };
	    @arval = @{$hr->{vals}};
	    push @arval, @whereval;
	}else { 
	    errRet("Error: unknown command modify $hr->{cmd}", '', $arg) 
	};
	
	# do query
	    $arg->{query} .= '=======' . $qr;
	    $arg->{bind} .= '=======' . join ',',@arval;
	
	my $sth = $dbh->prepare($qr) or $err = $dbh->errstr;
	if( $err ){
	    $dbh->rollback;
	    errRet( "Error execution $hr->{cmd} command on $hr->{table}:", $err, $arg );
	}
	$sth->execute(@arval) or $err = $dbh->errstr;
	if( $err ){
	    $dbh->rollback;
	    errRet( "Error execution $hr->{cmd} command on $hr->{table}:", $err, $arg );
	}
    }
    $dbh->commit;
    $arg->{status} = 0;
    $dbh->disconnect;
    return $arg;
}
#####################################################################
#	Return status and error string 
#####################################################################
sub errRet{
    my ( $errmsg, $nativemsg,$hout ) = @_;
    $hout->{retMsg} = $errmsg;
    $hout->{retNative} = $nativemsg;
    $hout->{status} = 1;
    print to_json($hout, {utf8 => 1});
    exit;
}
#####################################################################
#	Read ini file for database
#####################################################################
sub readIni{
    my $fname = shift;
    open(INP, $fname) or return "Can't open file $fname\n";
    while( my $s = <INP> ){
	chomp $s;
	my ( $name, $val ) = split ' ', $s;
	$ora{$name} = $val;
    }
    close(INP);
    return 0;
}
