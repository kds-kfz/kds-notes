#!/usr/bin/env perl
use Encode;
use JSON;
use Data::Dumper;
use Cwd;

=pod
example:
{
    "ptjy/ptyw":{
        "login":"ptjy_ptyw_login",
        "cccx":"ptjy_ptyw_cccx"
    }
}

## 说明:
## 这是一个读取kd-fs-done.businessid文件，生成business-map.json配置文件的脚本
## 用于引擎v2.0加载动态库 
=cut
$var = <<"EOF";
example:
{
    "ptjy/ptyw":{
        "login":"ptjy_ptyw_login",
        "cccx":"ptjy_ptyw_cccx"
    }
}

* 说明:
    * 这是一个读取kd-fs-done.businessid文件，生成business-map.json配置文件的脚本
    * 用于引擎v2.0加载动态库 
EOF
print "$var\n";

$input_file = 'kd-fs-done.businessid';
$output_file = 'business-map.json';

# 获取绝对路径
my $getpath = getcwd;
my $input_file_path = $getpath ."/" .$input_file;
my $output_file_path = $getpath ."/" .$output_file;

$json = JSON->new->utf8;
#验证 utf8 标志位
#print "utf8 flag: ",$json->get_utf8,"\n";

# 建个列表 如果只建字典 只能初始化 不能插入新键值
my $js;
my $obj;
my %hash= ();
my @arry = qw();
my $FILE = shift;

# 只读打开
open(FILE, "<$input_file_path") or die "Cannot open $input_file_path: $!\n"; 
while(defined($line = <FILE>)){
# 读取每行内容追加到数组
    push (@arry,$line)
}
close $FILE;

#foreach (@arry){
#    print "$_";
#}

foreach $line (@arry){
    if($line =~ /([a-zA-Z]+)\/([a-zA-Z]+)\/([a-zA-Z]+)/){
#        $hash{"$1/$2"}[0]{"$3"} = "$1_$2_$3";
        $hash{"$1/$2"}{"$3"} = "$1_$2_$3";
    }
}

print "统计接口总数: ";
print scalar @arry;
print "\n";

print "统计业务模块总数: ";
$length = keys %hash;
print "$length\n";

# 所有业务接口输出到终端
#print "array ref here : ", $json->encode(\%hash),"\n";
my $encode_json = encode_json( \%hash );
#print $encode_json; #打印json数据结构和数据
#print "\n";

#open(FILE, ">$output_file_path") or die "Opening $output_file_path: $!";
open FILE, '>', $output_file_path or die "Can't open file $output_file_path: $!\n";
if (! print FILE $encode_json, "\n"){
    warn "Unable to write to the $output_file_path: $!";
}
close(FILE);

# 读json文件
if(open(FILE,"$output_file_path")){
    while(<FILE>){
        $js .= "$_";
    }
    $obj = $json->decode($js);
#    print Dumper($obj)."\n";
    close(FILE);
}else{
    die("打开json数据失败！\n");
}

# 重新生成json文件
open FILE, '>', $output_file_path or die "Can't open file $output_file_path: $!\n";
if(! print FILE Dumper($obj). "\n"){
    warn "Unable to write to the $output_file_path: $!";
}
close(FILE);

my $command = "sed -i 's/=>/\:/' $output_file_path";
system($command);
print "配置文件已生成\n";

