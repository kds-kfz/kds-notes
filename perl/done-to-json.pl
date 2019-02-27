#!/usr/bin/env perl
use Encode;
use JSON;

$input_file = 'kd-fs-done.businessid';
$output_file = 'business-map.json';

$json = JSON->new->utf8;
#验证 utf8 标志位
print "utf8 flag: ",$json->get_utf8,"\n";

my $hash= {};
my @arry = qw();
my $FILE = shift;

# 只读打开
open(FILE, "<$input_file") or die "Cannot open myfile: $!\n"; 
while(defined($line = <FILE>)){
    push (@arry,$line)
}
close $FILE;

#foreach (@arry){
#    print "$_";
#}
print scalar @arry;
foreach $line (@arry){
    if($line =~ /([a-zA-Z]+)\/([a-zA-Z]+)\/([a-zA-Z]+)/){
#         %hash = {"$3"=>"$1_$2_$3"};
            $hash = {"$1/$2"=>{"$3"=>"$1_$2_$3"}};
    }
}
my $hash_1 ={
    "ptjy/ptyw"=>{
        "login"=>"ptjy_ptyw_login",
        "cccx"=>"ptjy_ptyw_cccx"
    },
    "rzrq/ptyw"=>{
        "login"=>"ptjy_ptyw_login",
        "cccx"=>"ptjy_ptyw_cccx"
    }
};
push @hash_1 , {"ptjy/ptyw3"=>1222222};

my $json_text = $json->encode ($hash_1);
print $json_text;
print "\n";

$some_hash = {
    "key4" => 1,
    "key5" => 2,
    "key6" => 3,
    "key7" => 4,
};


my $json_text = $json->encode ($some_hash);
print $json_text;
print "\n";


