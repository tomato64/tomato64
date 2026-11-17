<?php
/**
 * AdminNeo - Powerful database manager in a single PHP file
 * v5.9.0
 *
 * Compiled with
 * drivers:   mysql
 * languages: en
 * themes:    default-blue
 * config:    no
 *
 * @link https://www.adminneo.org/
 *
 * @author Peter Knut
 * @author Jakub Vrana (https://www.vrana.cz/)
 *
 * @copyright 2007-2025 Jakub Vrána
 * @copyright 2024-2025 Peter Knut
 *
 * @license Apache License, Version 2.0 (https://www.apache.org/licenses/LICENSE-2.0)
 * @license GNU General Public License, version 2 (https://www.gnu.org/licenses/gpl-2.0.html)
 */namespace
AdminNeo;use
Exception;use
stdClass;use
PDO;use
PDOStatement;use
mysqli;use
mysqli_result;abstract
class
Plugin{protected$admin;protected$config;protected$settings;protected$locale;function
inject($ya,Config$Tb,Settings$O,Locale$Og){$this->admin=$ya;$this->config=$Tb;$this->settings=$O;$this->locale=$Og;}}abstract
class
Origin
extends
Plugin{private$errors=[];private
static$instance=null;static
function
create(array$Tb=[],array$kj=[]){if(self::$instance)die("Admin instance already exists.\n");$ya=new
static();if(!$Tb&&file_exists("adminneo-config.php")){$Tb=include_once("adminneo-config.php");if(!is_array($Tb)){$Tb=[];$Gg="href=https://github.com/adminneo-org/adminneo#configuration ".target_blank();$ya->addError(lang(0,"<b>adminneo-config.php</b>")." <a $Gg>".lang(1)."</a>");}}$Tb=new
Config($Tb);$O=new
Settings($Tb);if(!$kj&&file_exists("adminneo-plugins.php")){$kj=include_once("adminneo-plugins.php");if(!is_array($kj)){$kj=[];$Gg="href=https://github.com/adminneo-org/adminneo#plugins ".target_blank();$ya->addError(lang(0,"<b>adminneo-plugins.php</b>")." <a $Gg>".lang(1)."</a>");}}self::$instance=$kj?new
Pluginer($ya,$kj):$ya;$ya->inject(self::$instance,$Tb,$O,Locale::get());foreach($kj
as$jj)$jj->inject(self::$instance,$Tb,$O,Locale::get());return
self::$instance;}static
function
get(){if(!self::$instance)die("Admin instance not found. Create instance by Admin::create() method at first.\n");return
self::$instance;}protected
function
__construct(){}function
getConfig(){return$this->config;}function
getSettings(){return$this->settings;}abstract
function
getOperators();function
getLikeOperator(){return
Driver::get()->getLikeOperator();}function
getRegexpOperator(){return
null;}function
init(){}function
addError($j){$this->errors[]=$j;}function
getErrors(){return$this->errors;}abstract
function
getServiceTitle();function
getCredentials(){$N=$this->config->getServer(SERVER);return[$N?$N->getServer():SERVER,$_GET["username"],get_password()];}function
verifyDefaultPassword($F){$Ue=$this->config->getDefaultPasswordHash();if($Ue===null||$Ue==="")return
lang(2);elseif(!password_verify($F,$Ue))return
lang(3);return
true;}function
authenticate($V,$F){if($F==""){$Ue=$this->config->getDefaultPasswordHash();if($Ue===null)return
lang(4,target_blank());else
return$Ue==="";}return
true;}function
getPrivateKey($e=false){return
get_private_key($e);}function
getBruteForceKey(){return$_SERVER["REMOTE_ADDR"];}function
getServerName($N,$ak=true,$Td=null){if($N==""){if(!$ak)return"";$N=Connection::exists()?Connection::get()->getDefaultServerName():"";if($N=="")return$Td!==null?$Td:lang(5);$Kk=null;}else$Kk=$this->config->getServer($N);return$Kk?$Kk->getName():preg_replace('~^https?://~',"",$N);}abstract
function
getDatabase();function
getDatabases($me=true){$g=$this->filterListWithWildcards(get_databases($me),$this->config->getHiddenDatabases(),false,Driver::get()->getSystemDatabases());if(DB!=""&&!in_array(DB,$g))array_unshift($g,DB);return$g;}function
getSchemas($Ph=false){$af=$this->config->getHiddenSchemas();if($Ph&&!in_array("__system",$af))$af[]="__system";$vk=$this->filterListWithWildcards(schemas(),$af,false,Driver::get()->getSystemSchemas());if(isset($_GET["ns"])&&$_GET["ns"]!=""&&!in_array($_GET["ns"],$vk))array_unshift($vk,$_GET["ns"]);return$vk;}function
getCollations(array$dg=[]){$on=$this->config->getVisibleCollations();$ge=$on?array_merge($on,$dg):[];return$this->filterListWithWildcards(collations(),$ge,true);}private
function
filterListWithWildcards(array$fn,array$ge,$fg,array$Fl=[]){if(!$fn||!$ge)return$fn;$s=array_search("__system",$ge);if($s!==false){unset($ge[$s]);$ge=array_merge($ge,$Fl);}array_walk($ge,function(&$Y){$Y=str_replace('\\*',".*",preg_quote($Y,"~"));});$cj='~^('.implode("|",$ge).')$~';return$this->filterListWithPattern($fn,$cj,$fg);}private
function
filterListWithPattern(array$fn,$cj,$fg){$I=[];foreach($fn
as$u=>$Y){if(is_array($Y)){if($wl=$this->filterListWithPattern($Y,$cj,$fg))$I[$u]=$wl;}elseif(($fg&&preg_match($cj,$Y))||(!$fg&&!preg_match($cj,$Y)))$I[$u]=$Y;}return$I;}abstract
function
getQueryTimeout();function
sendHeaders(){}function
updateCspHeader(array&$kc){}function
printFavicons(){$Eb=validate_color_variant($this->config->getColorVariant());echo"<link rel='icon' type='image/x-icon' href='",link_files("favicon-$Eb.ico",[]),"' sizes='32x32'>\n","<link rel='icon' type='image/svg+xml' href='",link_files("favicon-$Eb.svg",[]),"'>\n","<link rel='apple-touch-icon' href='",link_files("apple-touch-icon-$Eb.png",[]),"'>\n";}abstract
function
printToHead();function
getCssUrls(){$Tm=$this->config->getCssUrls();foreach(["adminneo.css","adminneo-light.css","adminneo-dark.css"]as$n){if(file_exists($n))$Tm[]="$n?v=".filemtime($n);}return$Tm;}function
isLightModeForced(){return$this->isColorSchemeForced(false);}function
isDarkModeForced(){return$this->isColorSchemeForced(true);}private
function
isColorSchemeForced($pc){$th=$pc?Settings::$ColorSchemeDark:Settings::$ColorSchemeLight;$uh=$pc?Settings::$ColorSchemeLight:Settings::$ColorSchemeDark;$ce=file_exists("adminneo-$th.css");$de=file_exists("adminneo-$uh.css");if($ce&&!$de)return
true;return$this->settings->getColorScheme()==$th&&!($ce
xor$de);}function
getJsUrls(){$Tm=$this->config->getJsUrls();$n="adminneo.js";if(file_exists($n))$Tm[]="$n?v=".filemtime($n);return$Tm;}abstract
function
printLoginForm();function
getLoginFormRow($Xd,$ng,$k){if($ng)return"<tr><th>$ng</th><td>$k</td></tr>\n";else
return"$k\n";}function
printLogout(){echo"<div class='logout'>","<form action='' method='post'>\n","<span title='",lang(6),"'>",h($_GET["username"]),"</span>","<input type='submit' class='button' name='logout' value='",lang(7),"' id='logout'>",input_token(),"</form>","</div>\n";}function
getTableName(array$Jl){return
h($Jl["Name"]);}abstract
function
getFieldName(array$k,$D=0);function
formatComment($Mb){return
h($Mb);}abstract
function
printTableMenu(array$Jl,$Df);function
getForeignKeys($Q){return
foreign_keys($Q);}function
getBackwardKeys($Q,$Hl){if(!$this->settings->isRelationLinks())return[];$L=backward_keys($Q);$hg=[];foreach($L
as$K){$r=$K["table_schema"].".".$K["table_name"];$hg[$r]["schema"]=$K["table_schema"];$hg[$r]["table"]=$K["table_name"];$hg[$r]["constraints"][$K["constraint_name"]][$K["column_name"]]=$K["referenced_column_name"];}foreach($hg
as$r=>$u){$A=$this->admin->getTableName(table_status1($u["table"],true));if($A!=""){$xk=preg_quote($Hl);$Hk="(:|\\s*-)?\\s+";$hg[$r]["name"]=(preg_match("(^$xk$Hk(.+)|^(.+?)$Hk$xk\$)iu",$A,$z)?$z[2].$z[3]:$A);}else
unset($hg[$r]);}return$hg;}function
printBackwardKeys(array$Ua,array$K){foreach($Ua
as$u){foreach($u["constraints"]as$Wb){$eh=preg_replace('~&ns=[^&]+&~',"&ns=".urldecode($u["schema"])."&",ME);$x=$eh.'select='.urlencode($u["table"]);$q=0;foreach($Wb
as$b=>$X){if(!isset($K[$X]))continue
2;$x
.=where_link($q++,$b,$K[$X]);}$A=preg_replace('(^'.preg_quote($_GET["select"]).(substr($_GET["select"],-1)=="s"?"?":"").'_)',"_",$u["name"]);$T=implode(", ",array_keys($Wb));echo"<a href='".h($x)."' title='".h($T)."'>".h($A)."</a>";$x=$eh.'edit='.urlencode($u["table"]);foreach($Wb
as$b=>$X)$x
.="&preset".urlencode("[".bracket_escape($b)."]")."=".urlencode($K[$X]);echo"<a href='".h($x)."' title='".lang(8)."'>",icon_solo("add"),"</a> ";}}}abstract
function
formatSelectQuery($H,$ml,$Sd=false);abstract
function
formatMessageQuery($H,$im,$Sd=false);abstract
function
formatSqlCommandQuery($H);function
printAfterSqlCommand(){}abstract
function
getTableDescriptionFieldName($Q);abstract
function
fillForeignDescriptions(array$L,array$pe);function
getFieldValueLink($X,$k){if(is_mail($X))return"mailto:$X";if(is_web_url($X))return$X;return
null;}abstract
function
formatSelectionValue($X,$x,$k,$Fi);abstract
function
formatFieldValue($Y,array$k);abstract
function
printTableStructure(array$l);abstract
function
printTablePartitions(array$Si);abstract
function
printRelatedTables(array$S);abstract
function
printTableIndexes(array$t,array$Jl);abstract
function
printSelectionColumns(array$M,array$c);abstract
function
printSelectionSearch(array$Z,array$c,array$t);abstract
function
printSelectionOrder(array$D,array$c,array$t);abstract
function
printSelectionLimit($w);abstract
function
printSelectionLength($dm);abstract
function
printSelectionAction(array$t);function
isDataEditAllowed(){return!information_schema(DB);}abstract
function
processSelectionColumns(array$c,array$t);abstract
function
processSelectionSearch(array$l,array$t);abstract
function
processSelectionOrder(array$l,array$t);function
processSelectionLimit(){if(!isset($_GET["limit"]))return$this->settings->getRecordsPerPage();return$_GET["limit"]!=""?(int)$_GET["limit"]:0;}abstract
function
processSelectionLength();abstract
function
getFieldFunctions(array$k);abstract
function
getFieldInput($Q,array$k,$Ma,$Y,$p);function
getFieldInputHint($Q,array$k,$Y){return
support("comment")?$this->admin->formatComment($k["comment"]):"";}abstract
function
processFieldInput(array$k,$Y,$p="");function
detectJson($Yd,&$Y,$vj=null){if(is_array($Y)){$ke=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($this->config->isJsonValuesAutoFormat()?JSON_PRETTY_PRINT:0);$Y=json_encode($Y,$ke);return
true;}$ke=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($vj?JSON_PRETTY_PRINT:0);if(preg_match('~^jsonb?$~',$Yd)){if($Y!=null&&$vj!==null&&$this->config->isJsonValuesAutoFormat()&&($bg=json_decode($Y))!==null)$Y=json_encode($bg,$ke);return
true;}if(!$this->config->isJsonValuesDetection())return
false;if(is_string($Y)&&$Y!=""&&preg_match('~varchar|text|character varying|String|keyword~',$Yd)&&($Y[0]=="{"||$Y[0]=="[")&&($bg=json_decode($Y))){if($vj!==null&&$this->config->isJsonValuesAutoFormat())$Y=json_encode($bg,$ke);return
true;}return
false;}function
getServerVariables(){return
show_variables();}function
getStatusVariables(){return
show_status();}function
getDumpFormats(){return['csv'=>['name'=>'CSV,','data'=>true],'csv;'=>['name'=>'CSV;','data'=>true],'tsv'=>['name'=>'TSV','data'=>true],];}function
getStructureDumpFormats(){return$this->filterDumpFormats("structure");}function
getDataDumpFormats(){return$this->filterDumpFormats("data");}private
function
filterDumpFormats($U){$te=[];foreach(self::get()->getDumpFormats()as$se=>$Cc){if(is_string($Cc))$te[$se]=$Cc;elseif(isset($Cc[$U])?$Cc[$U]:false)$te[$se]=isset($Cc["name"])?$Cc["name"]:$se;}return$te;}abstract
function
sendDumpFormatHeaders($lf,$yh=false);abstract
function
getDumpOutputs();abstract
function
sendDumpOutputHeaders($lf,$Od);function
sendDumpHeaders($lf,$yh=false){return
null;}function
dumpDatabase($tc){}abstract
function
dumpTable($Q,$vl,$ln=0);abstract
function
dumpData($Q,$vl,$H);abstract
function
getImportFilePath();abstract
function
printDatabaseMenu();abstract
function
printNavigation($rh);abstract
function
printDatabaseSwitcher($rh);function
printTablesFilter(){echo"<div class='tables-filter jsonly'>"."<input id='tables-filter' type='search' class='input' autocomplete='off' placeholder='".lang(9)."'>".script("initTablesFilter(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");")."</div>\n";}abstract
function
printTableList(array$S);function
getSettingsRows($Me){$O=[];if($Me==1){$C=get_language_options();if($C)$O["lang"]="<tr><th id='label-language'>".lang(10)."</th>"."<td>".html_select("lang",get_language_options(),Locale::get()->getLanguage(),"","label-language")."</td></tr>\n";$nm=get_theme_titles($this->config->getColorVariant());if(count($nm)>1){list($fm)=validate_theme($this->config->getTheme(),$this->config->getColorVariant());$C=[""=>lang(11)." ($nm[$fm])"]+$nm;$O["theme"]="<tr><th id='label-theme'>".lang(12)."</th>"."<td>".html_select("theme",$C,($ra=$this->settings->getParameter("theme"))!==null?$ra:"","","label-theme")."</td></tr>\n";}$C=[""=>lang(13),Settings::$ColorSchemeLight=>lang(14),Settings::$ColorSchemeDark=>lang(15)];$O["colorScheme"]="<tr><th>".lang(16)."</th>"."<td>".html_radios("colorScheme",$C,($ra=$this->settings->getParameter("colorScheme"))!==null?$ra:"")."</td></tr>\n";}elseif($Me==2){$C=[""=>lang(11),true=>lang(17),false=>lang(18),];$i=$C[$this->config->isRelationLinks()];$C[""].=" ($i)";$O["relationLinks"]="<tr><th>".lang(19)."</th>"."<td>".html_radios("relationLinks",$C,($ra=$this->settings->getParameter("relationLinks"))!==null?$ra:"")."<span class='input-hint'>".lang(20)."</span>"."</td></tr>\n";$i=$this->config->getRecordsPerPage();$C=[""=>lang(11)." ($i)","20","30","50","70","100",];$O["recordsPerPage"]="<tr><th id='label-records'>".lang(21)."</th>"."<td>".html_select("recordsPerPage",$C,($ra=$this->settings->getParameter("recordsPerPage"))!==null?$ra:"","","label-records")."<span class='input-hint'>".lang(22)."</span>"."</td></tr>\n";$i=($ra=$this->config->getEnumAsSelectThreshold())!==null?$ra:lang(23);$C=[""=>lang(11)." ($i)",-1=>lang(23),0=>lang(24),3=>lang(25,3),5=>lang(25,5),10=>lang(25,10),20=>lang(25,20),];$O["enumAsSelectThreshold"]="<tr><th id='label-enum'>".lang(26)."</th>"."<td>".html_select("enumAsSelectThreshold",$C,($ra=$this->settings->getParameter("enumAsSelectThreshold"))!==null?$ra:"","","label-enum",true)."<span class='input-hint'>".lang(27)."</span>"."</td></tr>\n";}return$O;}abstract
function
getForeignColumnInfo(array$pe,$b);}class
Pluginer{private
static$InternalMethods=["inject"=>true,"getConfig"=>true,];private
static$AppendMethods=["getErrors"=>true,"getFieldFunctions"=>true,"getDumpOutputs"=>true,"getDumpFormats"=>true,"getSettingsRows"=>true,];private$plugins;private$hooks=[];function
__construct(Origin$ya,array$kj){$this->plugins=$kj;foreach(get_class_methods('\AdminNeo\Origin')as$ph){$this->hooks[$ph]=[];if(!(isset(self::$InternalMethods[$ph])?self::$InternalMethods[$ph]:false)){foreach($kj
as$jj){if(method_exists($jj,$ph))$this->hooks[$ph][]=$jj;}}if(isset(self::$AppendMethods[$ph])?self::$AppendMethods[$ph]:false)array_unshift($this->hooks[$ph],$ya);else$this->hooks[$ph][]=$ya;}}function
getPlugins(){return$this->plugins;}function
__call($A,array$Ni){$Ha=isset(self::$AppendMethods[$A])?self::$AppendMethods[$A]:false;$I=$Ha?[]:null;assert(isset($this->hooks[$A]),"Calling unknown plugin method: $A");foreach($this->hooks[$A]as$jj){$Y=call_user_func_array([$jj,$A],$Ni);if($Y!==null){if($Ha)$I+=$Y;else
return$Y;}}return$I;}function
updateCspHeader(array&$kc){$this->__call(__FUNCTION__,[&$kc]);}function
detectJson($Yd,&$Y,$vj=null){return$this->__call(__FUNCTION__,[$Yd,&$Y,$vj]);}}class
Admin
extends
Origin{function
getOperators(){return
Driver::get()->getOperators();}function
getServiceTitle(){return"<a href='".h(HOME_URL)."'><svg role='img' class='logo' width='133' height='28'><desc>AdminNeo</desc>"."<use href='".link_files("logo.svg",[])."#logo'/></svg></a>";}function
getDatabase(){return
DB;}function
getQueryTimeout(){return
2;}function
printToHead(){echo"<link rel='stylesheet' href='",link_files("jush.css",[]),"'>";if(!$this->admin->isLightModeForced())echo"<link rel='stylesheet' ".(!$this->admin->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("jush-dark.css",[]),"'>\n";echo
script_src(link_files("jush.js",[]),true);}function
printLoginForm(){$bd=Drivers::getList();$Lk=$this->config->getServerPairs($bd);$N=SERVER?:$this->config->getDefaultServer();echo"<table class='box box-light'>\n";if($Lk)echo$this->admin->getLoginFormRow('server',lang(5),"<select name='auth[server]'>".optionlist($Lk,$N,true)."</select>");else{$Zc=DRIVER?:$this->config->getDefaultDriver($bd);if(count($bd)>1)echo$this->admin->getLoginFormRow('driver',lang(28),html_select("auth[driver]",$bd,$Zc).script("initLoginDriver(qsl('select'));",""));else
echo$this->admin->getLoginFormRow('driver','',input_hidden("auth[driver]",$Zc));echo$this->admin->getLoginFormRow('server',lang(5),"<input class='input' name='auth[server]' value='".h($N)."' title='".lang(29)."' placeholder='localhost' autocapitalize='off'>");}echo$this->admin->getLoginFormRow('username',lang(6),'<input class="input" name="auth[username]" id="username" value="'.h($_GET["username"]).'" autocomplete="username" autocapitalize="off">'),$this->admin->getLoginFormRow('password',lang(30),'<input type="password" class="input" name="auth[password]" autocomplete="current-password">');if(!$Lk){$tc=isset($_GET["db"])?$_GET["db"]:$this->config->getDefaultDatabase();echo$this->admin->getLoginFormRow('db',lang(31),'<input class="input" name="auth[db]" value="'.h($tc).'" autocapitalize="off">');}echo"</table>\n","<p>","<input type='submit' class='button default' value='".lang(32)."'>",checkbox("auth[permanent]",1,$_COOKIE["neo_permanent"],lang(33)),"</p>\n";}function
getFieldName(array$k,$D=0){$U=$k["full_type"].($k["null"]?" NULL":"");$Mb=$k["comment"];$Hk=$U&&$Mb!=""?": ":"";return'<span title="'.h($U.$Hk.$Mb).'">'.h($k["field"]).'</span>';}function
printTableMenu(array$Jl,$Df){echo'<p class="links top-tabs">';$Hg=[];$Dk=($this->settings->isSelectionPreferred()&&!$this->settings->isNavigationReversed())||(!$this->settings->isSelectionPreferred()&&$this->settings->isNavigationReversed());if($Dk)$Hg["select"]=[lang(34),"data"];if(support("table")||support("indexes"))$Hg["table"]=[lang(35),"structure"];if(!$Dk)$Hg["select"]=[lang(34),"data"];$Q=$Jl["Name"];$Wf=false;if(support("table")){$Wf=is_view($Jl);if(!$Wf){if($Q!="")$Hg["create"]=[lang(36),"edit"];}elseif(support("view"))$Hg["view"]=[lang(37),"edit"];}if($Df!==null)$Hg["edit"]=[lang(8),"item-add"];$Ni=$Df?"&".http_build_query($Df):"";foreach($Hg
as$u=>$X)echo" <a href='",h(ME),"$u=",urlencode($Q),($u=="edit"?$Ni:""),"'",bold(isset($_GET[$u])),">",icon($X[1]),"$X[0]</a>";echo
doc_link([DIALECT=>Driver::get()->tableHelp($Q,$Wf)],icon("help").lang(38)),"\n";}function
formatSelectQuery($H,$ml,$Sd=false){$Al=support("sql");$sn=!$Sd?Driver::get()->warnings():null;if($Al)$H
.=";";$Dl=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J="<pre><code class='jush-$Dl'>".h(str_replace("\n"," ",$H))."</code></pre>\n";$J
.="<p class='links'>";if($Al)$J
.="<a href='".h(ME)."sql=".urlencode($H)."'>".icon("edit").lang(39)."</a>";if($sn)$J
.="<a href='#warnings' class='toggle'>".lang(40).icon_chevron_down()."</a>";$J
.=" <span class='time'>(".format_time($ml).")</span>";$J
.="</p>\n";if($sn){$J
.=script("initToggles(qsl('p'));");$J
.="<div id='warnings' class='warnings hidden'>\n$sn\n</div>\n";}return$J;}function
formatMessageQuery($H,$im,$Sd=false){restart_session();$cf=&get_session("queries");if(!isset($cf[$_GET["db"]]))$cf[$_GET["db"]]=[];if(strlen($H)>1e6)$H=preg_replace('~[\x80-\xFF]+$~','',substr($H,0,1e6))."\n…";$cf[$_GET["db"]][]=[$H,time(),$im];$Al=support("sql");$sn=!$Sd?Driver::get()->warnings():null;$il="sql-".count($cf[$_GET["db"]]);$tn="warnings-".count($cf[$_GET["db"]]);$J=" ";if($sn)$J
.="<a href='#$tn' class='toggle'>".lang(40).icon_chevron_down()."</a>, ";$Gj=support("sql")?lang(41):lang(42);$J
.="<a href='#$il' class='toggle'>$Gj".icon_chevron_down()."</a>";$J
.=" <span class='time'>".@date("H:i:s")."</span>\n";if($sn)$J
.="<div id='$tn' class='warnings hidden'>\n$sn</div>\n";$J
.="<div id='$il' class='hidden'>\n";$Dl=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J
.="<pre><code class='jush-$Dl'>".truncate_utf8($H,1000)."</code></pre>\n";$J
.="<p class='links'>";if($Al)$J
.="<a href='".h(str_replace("db=".urlencode(DB),"db=".urlencode($_GET["db"]),ME).'sql=&history='.(count($cf[$_GET["db"]])-1))."'>".icon("edit").lang(39)."</a>";if($im)$J
.=" <span class='time'>($im)</span>";$J
.="</p>\n";$J
.="</div>\n";return$J;}function
formatSqlCommandQuery($H){if(preg_match('~^DELIMITER\s~i',$H))return"";return
truncate_utf8($H,1000);}function
getTableDescriptionFieldName($Q){return"";}function
fillForeignDescriptions(array$L,array$pe){return$L;}function
formatSelectionValue($X,$x,$k,$Fi){if($X===null)$cm="<i>NULL</i>";elseif(!$k)$cm=$X;elseif(preg_match("~char|binary|boolean~",$k["type"])&&!preg_match("~var~",$k["type"]))$cm="<code>$X</code>";elseif(is_blob($k)&&!is_utf8($X))$cm="<i>".lang(43,strlen($Fi))."</i>";elseif($this->admin->detectJson($k["full_type"],$Fi))$cm="<code class='jush-json'>$X</code>";else$cm=$X;if($x)$cm="<a href='".h($x)."'".(is_web_url($x)?target_blank():"").">$cm</a>";return$cm;}function
formatFieldValue($Y,array$k){return$Y;}function
printTableStructure(array$l){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th>",lang(44),"</th>","<td>",lang(45)," <span class='note'>[".lang(46)."]</span></td>","<td>",lang(47),"</td>";if(support("comment"))echo"<td>",lang(48),"</td>";echo"</tr></thead>\n";$an=Driver::get()->getUserTypes();foreach($l
as$k){echo"<tr>","<th>",h($k["field"]),"</th>","<td>";$U=h($k["full_type"]);if(in_array($U,$an))echo"<a href='".h(ME.'type='.urlencode($U))."'>$U</a>";else
echo$U;if($k["null"])echo" <i>NULL</i>";if($k["auto_increment"])echo" <i>".lang(49)."</i>";if(isset($k["default"])){if($k["generated"])echo" <i>AS</i> <code class='jush-".DIALECT."'>".h(parenthesize_expression($k["default"]))."</code>"," <i>".h($k["generated"])."</i>";else
echo" [<strong>".h($k["default"])."</strong>]";}echo"</td>","<td>",h($k["collation"]),"</td>";if(support("comment"))echo"<td>",$this->admin->formatComment($k["comment"]),"</td>";echo"\n";}echo"</table>\n","</div>\n";}function
printTablePartitions(array$Si){$Vk=isset($Si["partition_names"]);echo"<p>","<code class='jush-".DIALECT."'>BY {$Si["partition_by"]} ({$Si["partition"]})</code>";if(!$Vk&&isset($Si["partitions"]))echo" ".lang(50).": ".h($Si["partitions"]);echo"</p>";if($Vk){echo"<table>\n","<thead><tr><th>".lang(51)."</th><td>".lang(52)."</td></tr></thead>\n";foreach($Si["partition_names"]as$u=>$A){echo"<tr><th>";if(DIALECT=="pgsql")echo"<a href='",h(ME."table=".urlencode($A)),"'>";echo
h($A);if(DIALECT=="pgsql")echo"</a>";echo"</th><td>".h($Si["partition_values"][$u])."\n";}echo"</table>\n";}}function
printRelatedTables(array$S){echo"<ul class='links'>\n";foreach($S
as$K){$x=preg_replace('~ns=[^&]*~',"ns=".urlencode($K["ns"]),ME);echo"<li><a href='",h($x."table=".urlencode($K["table"])),"'>",icon("structure");if($K["ns"]!=$_GET["ns"])echo"<b>".h($K["ns"])."</b>.";echo
h($K["table"]),"</a>";}echo"</ul>\n";}function
printTableIndexes(array$t,array$Jl){$zc=first(Driver::get()->getIndexAlgorithms($Jl));$Qi=false;foreach($t
as$s){if(isset($s["partial"])?$s["partial"]:false){$Qi=true;break;}}echo"<table>\n","<thead><tr>","<th>",lang(45),"</th>","<td>",lang(53)," <span class='note'>(",lang(54),")</span></td>";if($Qi)echo"<td>",lang(55),"</td>";echo"</tr></thead>\n";foreach($t
as$A=>$s){ksort($s["columns"]);$xj=[];foreach($s["columns"]as$u=>$X)$xj[]="<i>".h($X)."</i>".($s["lengths"][$u]?"(".h($s["lengths"][$u]).")":"").($s["descs"][$u]?" DESC":"");echo"<tr title='",h($A),"'>","<th>",h($s["type"]);if(isset($s['algorithm'])&&$s['algorithm']!=$zc)echo" (",h($s['algorithm']),")";echo"</th>","<td>",implode(", ",$xj),"</td>";if($Qi){echo"<td>";if($s['partial'])echo"<code class='jush-",DIALECT,"'>WHERE ",h($s['partial']),"</code>";echo"</td>";}echo"</tr>\n";}echo"</table>\n";}function
printSelectionColumns(array$M,array$c){print_fieldset_start("select",lang(56),"columns",(bool)$M,true);$M[""]=[];$q=0;foreach($M
as$u=>$X){$X=isset($_GET["columns"][$u])?$_GET["columns"][$u]:[];$b=select_input("name='columns[$q][col]'",$c,isset($X["col"])?$X["col"]:null,$u!==""?"selectFieldChange":"selectAddRow");echo"<div ",($u!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly");if(Driver::get()->getFunctions()||Driver::get()->getGrouping())echo
html_select("columns[$q][fun]",[-1=>""]+array_filter([lang(57)=>Driver::get()->getFunctions(),lang(58)=>Driver::get()->getGrouping()]),isset($X["fun"])?$X["fun"]:null),help_script_command("value && value.replace(/ |\$/, '(') + ')'",true),script("qsl('select').onchange = (event) => { ".($u!==""?"":" qsl('select, input:not(.remove)', event.target.parentNode).onchange();")." };",""),"($b)";else
echo$b;echo" <button class='button light remove jsonly' title='",lang(59),"'>",icon_solo("remove"),"</button>",script("qsl('#fieldset-select .remove').onclick = selectRemoveRow;",""),"</div>\n";$q++;}print_fieldset_end("select",true);}function
printSelectionSearch(array$Z,array$c,array$t){print_fieldset_start("search",lang(60),"search",(bool)$Z);foreach($t
as$q=>$s){if($s["type"]=="FULLTEXT"){echo"<div>(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$s["columns"]))."</i>) AGAINST","<input type='text' class='input' name='fulltext[$q]' value='".h(isset($_GET["fulltext"][$q])?$_GET["fulltext"][$q]:null)."'>",script("qsl('input').oninput = selectFieldChange;","");if(DIALECT=='sql')echo
checkbox("boolean[$q]",1,isset($_GET["boolean"][$q]),"BOOL");echo"</div>\n";}}$mb="this.parentNode.firstChild.onchange();";foreach(array_merge((array)$_GET["where"],[[]])as$q=>$X){if(!$X||("$X[col]$X[val]"!=""&&in_array($X["op"],$this->getOperators())))echo"<div>",select_input(" name='where[$q][col]'",$c,$X["col"],($X?"selectFieldChange":"selectAddRow"),"(".lang(61).")"),html_select("where[$q][op]",$this->getOperators(),$X["op"],$mb),"<input type='text' class='input' name='where[$q][val]' value='".h($X["val"])."'>",script("mixin(qsl('input'), {oninput: function () { $mb }, onkeydown: selectSearchKeydown});","")," <button class='button light remove jsonly' title='".lang(59)."'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-search .remove").onclick = selectRemoveRow;',""),"</div>\n";}print_fieldset_end("search");}function
printSelectionOrder(array$D,array$c,array$t){print_fieldset_start("sort",lang(62),"sort",(bool)$D,true);$_GET["order"][""]="";$q=0;foreach((array)$_GET["order"]as$u=>$X){if($u!=""&&$X=="")continue;echo"<div ",($u!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly"),select_input("name='order[$q]'",$c,$X,$u!==""?"selectFieldChange":"selectAddRow")," ",checkbox("desc[$q]",1,isset($_GET["desc"][$u]),lang(63))," <button class='button light remove jsonly' title='",lang(59),"'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-sort .remove").onclick = selectRemoveRow;',""),"</div>\n";$q++;}print_fieldset_end("sort",true);}function
printSelectionLimit($w){echo"<fieldset><legend>".lang(64)."</legend><div class='fieldset-content'>","<input type='number' name='limit' class='input size' value='".h($w?:"")."'>",script("qsl('input').oninput = selectFieldChange;",""),"</div></fieldset>\n";}function
printSelectionLength($dm){if($dm!==null)echo"<fieldset><legend>".lang(65)."</legend><div class='fieldset-content'>","<input type='number' name='text_length' class='input size' value='".h($dm)."'>","</div></fieldset>\n";}function
printSelectionAction(array$t){echo"<fieldset><legend>".lang(66)."</legend><div class='fieldset-content'>","<input type='submit' class='button' value='".lang(56)."'>"," <span id='noindex' title='".lang(67)."'></span>","<script".nonce().">\n";$c=new
stdClass();foreach($t
as$s){$mc=reset($s["columns"]);if($s["type"]!="FULLTEXT"&&$mc)$c->$mc=null;}echo"const indexColumns = ".json_encode($c,JSON_UNESCAPED_UNICODE|JSON_HEX_TAG).";\n","selectFieldChange.call(gid('form')['select']);\n","</script>\n","</div></fieldset>\n";}function
processSelectionColumns(array$c,array$t){$M=[];$Ke=[];foreach((array)$_GET["columns"]as$u=>$X){if($X["fun"]=="count"||($X["col"]!=""&&(!$X["fun"]||in_array($X["fun"],Driver::get()->getFunctions())||in_array($X["fun"],Driver::get()->getGrouping())))){$M[$u]=apply_sql_function($X["fun"],($X["col"]!=""?idf_escape($X["col"]):"*"));if(!in_array($X["fun"],Driver::get()->getGrouping()))$Ke[]=$M[$u];}}return[$M,$Ke];}function
processSelectionSearch(array$l,array$t){$J=[];foreach($t
as$q=>$s){if($s["type"]=="FULLTEXT"&&isset($_GET["fulltext"])&&$_GET["fulltext"][$q]!="")$J[]="MATCH (".implode(", ",array_map('AdminNeo\idf_escape',$s["columns"])).") AGAINST (".q($_GET["fulltext"][$q]).(isset($_GET["boolean"][$q])?" IN BOOLEAN MODE":"").")";}foreach((array)$_GET["where"]as$Z){$Ab=$Z["col"];$ki=$Z["op"];$X=$Z["val"];if("$Ab$X"!=""&&in_array($ki,$this->getOperators())){if($ki=="SQL"&&(!$_POST||!verify_token()))Connection::get()->setUntrusted(true);$Sb=[];$Hf='-?\d+';$B='-?(\d+(\.\d*)?|\.\d+)';$Of=preg_match('~IN$~',$ki);$If="~^$Hf".($Of?"(,$Hf)*":"")."$~";$Uh="~^$B".($Of?"(,$B)*":"")."$~";$uc='~^(\d+)-(\d+)-(\d+)([ T]([01]?\d|2[0-3]):[0-5]\d(:[0-5]\d(\.\d+)?)?(\s*(z|utc|[+-](0?\d|1[0-5])(:?[0-5]\d)?))?)?$~i';foreach(($Ab!=""?[$Ab=>isset($l[$Ab])?$l[$Ab]:[]]:$l)as$A=>$k){$tj="";$Rb=" $ki";$Pf=preg_match('~'.number_type().(DIALECT=="pgsql"?"":'|bit').'~',$k["type"])&&!preg_match('~\[~',$k["full_type"]);$Zh=DIALECT=="pgsql"&&$ki=="="&&$k["type"]=="oid";if($Zh)$Rb
.=" ".$this->admin->processFieldInput($k,$X)."::regproc";elseif(preg_match('~IN$~',$ki)){$qf=process_length($X);if((!$Pf||(DIALECT=="pgsql"&&$k["type"]=="money"))&&preg_match('~^\([-+.\d,]+\)$~',$qf))$qf="(".implode(",",array_map('AdminNeo\q',explode(",",substr($qf,1,-1)))).")";$Rb
.=" ".($qf!=""?$qf:"(NULL)");}elseif($ki=="SQL")$Rb=" $X";elseif(preg_match('~^(I?LIKE) %%$~',$ki,$z))$Rb=" $z[1] ".$this->admin->processFieldInput($k,"%$X%");elseif($ki=="FIND_IN_SET"){$tj="$ki(".q($X).", ";$Rb=")";}elseif(!preg_match('~NULL$~',$ki))$Rb
.=" ".$this->admin->processFieldInput($k,$X);if($Ab!=""||(isset($k["privileges"]["where"])&&(preg_match(preg_match('~^'.int_type().'$~',$k["type"])?$If:$Uh,$X)||!$Pf)&&(!preg_match("~[\x80-\xFF]~",$X)||preg_match('~char|text|enum|set|keyword~',$k["type"]))&&(!preg_match('~date|timestamp~',$k["type"])||(preg_match($uc,$X,$z)&&checkdate((int)$z[2],(int)$z[3],(int)$z[1])))&&Driver::get()->isSearchable($Z,$k)&&(!preg_match('~^elastic~',DRIVER)||$k["type"]!="boolean"||preg_match('~true|false~',$X))&&(!preg_match('~^elastic~',DRIVER)||strpos($ki,"regexp")===false||preg_match('~text|keyword~',$k["type"]))&&(!preg_match('~^elastic~',DRIVER)||!preg_match('~^((unsigned_)?long|short|byte)$~',$k["type"])||preg_match('~^-?\d+$~',$X)))){if($Zh)$Sb[]=$tj.idf_escape($A).$Rb;else$Sb[]=$tj.Driver::get()->convertSearch(idf_escape($A),$Z,$k).$Rb;}}if(count($Sb)==1)$J[]=$Sb[0];elseif($Sb)$J[]="(".implode(" OR ",$Sb).")";else$J[]="1 = 0";}}return$J;}function
processSelectionOrder(array$l,array$t){$J=[];foreach((array)$_GET["order"]as$u=>$X){if($X!="")$J[]=(preg_match('~^((COUNT\(DISTINCT |[A-Z0-9_]+\()(`(?:[^`]|``)+`|"(?:[^"]|"")+")\)|COUNT\(\*\))$~',$X)?$X:idf_escape($X)).(isset($_GET["desc"][$u])?" DESC".(DIALECT=="pgsql"&&(isset($l[$X]["null"])?$l[$X]["null"]:null)?" NULLS LAST":""):"");}return$J;}function
processSelectionLength(){return
isset($_GET["text_length"])?$_GET["text_length"]:"100";}function
getFieldFunctions(array$k){$J=($k["null"]?"NULL/":"");$Qm=isset($_GET["select"])||where($_GET);foreach([Driver::get()->getInsertFunctions(),Driver::get()->getEditFunctions()]as$u=>$Ce){if(!$u||(!isset($_GET["call"])&&$Qm)){foreach($Ce
as$cj=>$X){if(!$cj||preg_match("~$cj~",$k["type"]))$J
.="/$X";}}if($u&&$Ce&&!preg_match('~enum|set|bool~',$k["type"])&&!is_blob($k))$J
.="/SQL";}if($k["auto_increment"]&&!$Qm)$J=lang(49);return
explode("/",$J);}function
getFieldInput($Q,array$k,$Ma,$Y,$p){return"";}function
processFieldInput(array$k,$Y,$p=""){if($p=="SQL")return$Y;$A=$k["field"];$J=q($Y);if(preg_match('~^(now|getdate|uuid)$~',$p))$J="$p()";elseif(preg_match('~^current_(date|timestamp)$~',$p))$J=$p;elseif(preg_match('~^([+-]|\|\|)$~',$p))$J=idf_escape($A)." $p $J";elseif(preg_match('~^[+-] interval$~',$p))$J=idf_escape($A)." $p ".(preg_match("~^(\\d+|'[0-9.: -]') [A-Z_]+\$~i",$Y)&&DIALECT!="pgsql"?$Y:$J);elseif(preg_match('~^(addtime|subtime|concat)$~',$p))$J="$p(".idf_escape($A).", $J)";elseif(preg_match('~^(md5|sha1|password|encrypt)$~',$p))$J="$p($J)";elseif($k["type"]=="boolean"&&DIALECT=="elastic")$J=$J=="0"?"false":"true";return
unconvert_field($k,$J);}function
getDumpFormats(){return(support("dump")?['sql'=>'SQL']:[])+parent::getDumpFormats();}function
sendDumpFormatHeaders($lf,$yh=false){$Od=(str_contains($_POST["format"],"sql")?"sql":($yh?"tar":"csv"));if($Od=="tar")header("Content-Type: application/x-tar");elseif($Od=="sql"||$_POST["output"]=="text")header("Content-Type: text/plain; charset=utf-8");else
header("Content-Type: text/csv; charset=utf-8");return$Od;}function
getDumpOutputs(){$Ji=['file'=>lang(68),'text'=>lang(69),];if(function_exists('gzencode'))$Ji['gz']='gzip';return$Ji;}function
sendDumpOutputHeaders($lf,$Od){if($_POST["output"]=="gz"){header("Content-Type: application/x-gzip");if(function_exists('deflate_init')){$Zb=deflate_init(ZLIB_ENCODING_GZIP);ob_start(function($P,$fj)use($Zb){return
deflate_add($Zb,$P,($fj&PHP_OUTPUT_HANDLER_END?ZLIB_FINISH:ZLIB_NO_FLUSH));},1e6);}else{ob_start(function($P){return
gzencode($P);},1e6);}}}function
dumpTable($Q,$vl,$ln=0){if($_POST["format"]!="sql"){echo"\xef\xbb\xbf";if($vl)dump_csv(array_keys(fields($Q)));}else{if($ln==2){$l=[];foreach(fields($Q)as$A=>$k)$l[]=idf_escape($A)." $k[full_type]";$e="CREATE TABLE ".table($Q)." (".implode(", ",$l).")";}else$e=create_sql($Q,$_POST["auto_increment"],$vl);set_utf8mb4($e);if($vl&&$e){if($vl=="DROP+CREATE"||$ln==1)echo"DROP ".($ln==2?"VIEW":"TABLE")." IF EXISTS ".table($Q).";\n";if($ln==1)$e=remove_definer($e);echo"$e;\n\n";}}}function
dumpData($Q,$vl,$H){if($vl){$Xg=(DIALECT=="sqlite"?0:1048576);$l=[];$mf=false;if($_POST["format"]=="sql"){if($vl=="TRUNCATE+INSERT")echo
truncate_sql($Q).";\n";$l=fields($Q);if(DIALECT=="mssql"){foreach($l
as$k){if($k["auto_increment"]){echo"SET IDENTITY_INSERT ".table($Q)." ON;\n";$mf=true;break;}}}}$I=Connection::get()->query($H,1);if($I){$Bf="";$eb="";$hg=[];$Ee=[];$yl="";$gc=0;while($K=($Q!=''?$I->fetchAssoc():$I->fetchRow())){if(!$hg){$fn=[];foreach($K
as$X){$k=$I->fetchField();if(!empty($l[$k->name]['generated'])){$Ee[$k->name]=true;continue;}$hg[]=$k->name;$u=idf_escape($k->name);$fn[]="$u = VALUES($u)";}$yl=($vl=="INSERT+UPDATE"?"\nON DUPLICATE KEY UPDATE ".implode(", ",$fn):"").";\n";}if($_POST["format"]!="sql"){if($vl=="table"){dump_csv($hg);$vl="INSERT";}dump_csv($K);}else{if(!$Bf)$Bf="INSERT INTO ".table($Q)." (".implode(", ",array_map('AdminNeo\idf_escape',$hg)).") VALUES";foreach($K
as$u=>$X){if(isset($Ee[$u])){unset($K[$u]);continue;}$k=$l[$u];$K[$u]=($X===null?"NULL":($X===false?0:unconvert_field($k,preg_match(number_type(),$k["type"])&&!preg_match('~\[~',$k["full_type"])&&is_numeric($X)?$X:(!is_blob($k)||is_utf8($X)?q($X):Driver::get()->quoteBinary($X)))));}$mk=($Xg?"\n":" ")."(".implode(",\t",$K).")";if(!$eb)$eb=$Bf.$mk;elseif(DIALECT=="mssql"?$gc%1000!=0:strlen($eb)+4+strlen($mk)+strlen($yl)<$Xg)$eb
.=",$mk";else{echo$eb.$yl;$eb=$Bf.$mk;}}$gc++;}if($eb)echo$eb.$yl;}elseif($_POST["format"]=="sql")echo"-- ".str_replace("\n"," ",Connection::get()->getError())."\n";if($mf)echo"SET IDENTITY_INSERT ".table($Q)." OFF;\n";}}function
getImportFilePath(){return"adminneo.sql";}function
printDatabaseMenu(){echo"<p class='links top-links'>\n";$Rh=isset($_GET["ns"])?$_GET["ns"]:null;if($Rh==""&&support("database"))echo'<a href="',h(ME),'database=">',icon("edit"),lang(70),"</a>\n";if($Rh!=""&&support("scheme"))echo"<a href='",h(ME),"scheme='>",icon("edit"),lang(71),"</a>\n";if($Rh!=="")echo'<a href="',h(ME),'schema=">',icon("schema"),lang(72),"</a>\n";if(support("privileges"))echo"<a href='",h(ME),"privileges='>",icon("users"),lang(73),"</a>\n";echo"</p>\n";}function
printNavigation($rh){if($rh=="auth"){$Ii="";foreach((array)$_SESSION["pwds"]as$hn=>$Pk){foreach($Pk
as$N=>$bn){foreach($bn
as$V=>$F){if($F!==null){$xc=$_SESSION["db"][$hn][$N][$V];foreach(($xc?array_keys($xc):[""])as$h){$Mk=$this->admin->getServerName($N,false);$T=h(get_driver_name($hn,$N)).($V!=""||$Mk!=""?" - ":"").h($V).($V!=""&&$Mk!=""?"@":"").h($Mk).($h!=""?h(" - $h"):"");$Ii
.="<li><a href='".h(auth_url($hn,$N,$V,$h))."' class='primary' title='$T'>$T</a></li>\n";}}}}}if($Ii)echo"<nav id='logins'><menu>\n$Ii</menu></nav>\n";}else{$this->admin->printDatabaseSwitcher($rh);$va=[];if(DB==""||!$rh){if(support("sql")){$va[]="<a href='".h(ME)."sql='".bold(isset($_GET["sql"])&&!isset($_GET["import"])).">".icon("command").lang(41)."</a>";$va[]="<a href='".h(ME)."import='".bold(isset($_GET["import"])).">".icon("import").lang(74)."</a>";}$va[]="<a href='".h(ME)."dump=".urlencode(isset($_GET["table"])?$_GET["table"]:$_GET["select"])."' id='dump'".bold(isset($_GET["dump"])).">".icon("export").lang(75)."</a>";}if(DB=="")$va[]='<a href="'.h(ME).'database="'.bold($_GET["database"]==="").">".icon("database-add").lang(76)."</a>\n";if(DB!=""&&$_GET["ns"]===""&&!$rh)$va[]='<a href="'.h(ME).'scheme="'.bold($_GET["scheme"]==="").">".icon("database-add").lang(77)."</a>\n";if(DB!=""&&$_GET["ns"]!==""&&!$rh)$va[]='<a href="'.h(ME).'create="'.bold($_GET["create"]==="").">".icon("table-add").lang(78)."</a>\n";if($va)echo"<p class='links'>".implode("\n",$va)."</p>";$S=[];if($_GET["ns"]!==""&&!$rh&&DB!=""){Connection::get()->selectDatabase(DB);$S=table_status('',true);}if($_GET["ns"]!==""&&!$rh&&DB!=""){if($S){$this->admin->printTablesFilter();$this->admin->printTableList($S);}else
echo"<div id='tables'><p>".lang(79)."</p></div>\n";}elseif($_GET["ns"]!==""&&DB!="")echo"<div id='tables'></div>\n";if(support("sql")||DIALECT=="elastic"||DIALECT=="mongo"){echo"<script".nonce().">\n";if(support("sql")&&$S){$Hg=[];foreach($S
as$Q=>$U)$Hg[]=js_escape_re($Q);$Il=support("table")&&!$this->config->isSelectionPreferred()?"table":"select";echo"window.jushLinks = { ".DIALECT.": {\n",js_escape_key(ME.$Il.'=$&'),': /\b(?<!\$)('.implode('|',$Hg).')(?!\$)\b/g';$jl=["sql","check","event","procedure","trigger","view","type","table","processlist"];if(support('routine')&&array_intersect_key($_GET,array_flip($jl))){foreach(routines()as$K)echo",\n",js_escape_key(ME.'function='.urlencode($K["SPECIFIC_NAME"]).'&name=$&'),': /\b'.js_escape_re($K["ROUTINE_NAME"]).'(?=["`\]]?\()/g';}echo"\n}};\n";foreach(["bac","bra","sqlite_quo","mssql_bra"]as$X)echo"jushLinks.$X = jushLinks.".DIALECT.";\n";}if(DIALECT!="elastic"&&DIALECT!="mongo"&&$this->getConfig()->isSqlAutocompletionEnabled()&&(isset($_GET["sql"])||isset($_GET["trigger"])||isset($_GET["check"]))){$Sl=array_fill_keys(array_keys($S),[]);foreach(Driver::get()->getAllFields()as$Q=>$l){foreach($l
as$k)$Sl[$Q][]=$k["field"];}echo"window.addEventListener('DOMContentLoaded', () => { autocompletion = jush.autocompleteSql('".idf_escape("")."', ".json_encode($Sl,JSON_HEX_TAG)."); });\n";}echo"</script>\n";}echo
script("let autocompletion;\nwindow.addEventListener('DOMContentLoaded', () => { initSyntaxHighlighting('".js_escape(doc_version())."', '".js_escape(Connection::get()->getFlavor())."', autocompletion); });");}}function
printDatabaseSwitcher($rh){$g=$this->admin->getDatabases();if(!$g&&DIALECT!="sqlite")return;echo"<div class='db-selector'><form action=''>";hidden_fields_get();echo"<div>";if($g)echo"<select id='database-select' name='db' title='",lang(31),"'>".optionlist([""=>"(".lang(80).")"]+$g,DB)."</select>".script("mixin(gid('database-select'), {onmousedown: dbMouseDown, onchange: dbChange});");else
echo"<input id='database-select' class='input' name='db' value='".h(DB)."' title='",lang(31),"' autocapitalize='off'>\n";echo"<input type='submit' value='".lang(81)."' class='button ".($g?"hidden":"")."'>\n","</div>";foreach(["import","sql","schema","dump","privileges"]as$X){if(isset($_GET[$X])){echo
input_hidden($X);break;}}echo"</form></div>\n";}function
printTableList(array$S){$gd=$this->settings->isNavigationDual()||$this->settings->isNavigationHover();$gh=($gd?"class='dual".($this->settings->isNavigationHover()?" hover":"")."'":($this->settings->isNavigationReversed()?"class='reversed'":""));echo"<nav id='tables'><div class='scroll-marker'></div><menu $gh>";foreach($S
as$Q=>$ol){$Q="$Q";$A=$this->admin->getTableName($ol);if($A==""||(isset($ol["Partition"])?$ol["Partition"]:false))continue;echo"<li>";$wa=in_array($Q,[$_GET["table"],$_GET["select"],$_GET["create"],$_GET["indexes"],$_GET["foreign"],$_GET["trigger"],$_GET["check"],$_GET["view"]]);$zb="primary".(is_view($ol)?" view":"");$Bl=support("table")||support("indexes");$_k=h(ME)."select=".urlencode($Q);$Kl=h(ME)."table=".urlencode($Q);if($this->settings->isSelectionPreferred()){if($this->settings->isNavigationReversed()&&$Bl)echo" <a href='$Kl' title='",lang(35),"' class='secondary'>",icon("structure"),"</a>";echo"<a href='$_k'",bold($wa,$zb)," data-primary='true' title='$A'>$A</a>";if($gd&&$Bl)echo" <a href='$Kl' title='",lang(35),"' class='secondary'>",icon_solo("structure"),"</a>";}else{if($this->settings->isNavigationReversed())echo" <a href='$_k' title='",lang(34),"' class='secondary'>",icon("data"),"</a>";if($Bl)echo"<a href='$Kl'",bold($wa,$zb)," data-primary='true' title='$A'>$A</a>";else
echo"<span data-primary='true'",bold($wa,$zb),">$A</span>";if($gd)echo" <a href='$_k' title='",lang(34),"' class='secondary'>",icon_solo("data"),"</a>";}echo"</li>\n";}echo"</menu></nav>\n",script("initTablesList(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");");}function
getSettingsRows($Me){$O=parent::getSettingsRows($Me);if($Me==1){$C=[""=>lang(11),Config::$NavigationSimple=>lang(82),Config::$NavigationDual=>lang(83),Config::$NavigationHover=>lang(84),Config::$NavigationReversed=>lang(85)];$i=$C[$this->config->getNavigationMode()];$C[""].=" ($i)";$O["navigationMode"]="<tr><th>".lang(86)."</th>"."<td>".html_radios("navigationMode",$C,($ra=$this->settings->getParameter("navigationMode"))!==null?$ra:"")."<span class='input-hint'>".lang(87)."</span>"."</td></tr>\n";$C=[""=>lang(11),0=>lang(35),1=>lang(34),];$i=$C[$this->config->isSelectionPreferred()?1:0];$C[""].=" ($i)";$O["preferSelection"]="<tr><th id='label-links'>".lang(88)."</th>"."<td>".html_select("preferSelection",$C,($ra=$this->settings->getParameter("preferSelection"))!==null?$ra:"","","label-links",true)."<span class='input-hint'>".lang(89)."</span>"."</td></tr>\n";}return$O;}function
getForeignColumnInfo(array$pe,$b){return
null;}}class
TmpFile{private$handler;private$size;function
__construct(){$this->handler=tmpfile();}function
getSize(){return$this->size;}function
write($Yb){if(!$this->handler)return;$this->size+=strlen($Yb);fwrite($this->handler,$Yb);}function
send(){if(!$this->handler)return;fseek($this->handler,0);fpassthru($this->handler);fclose($this->handler);}}function
print_select_result(Result$I,$d=null,array$_i=[],$w=0){$Hg=[];$t=[];$c=[];$ab=[];$Em=[];$J=[];for($q=0;(!$w||$q<$w)&&($K=$I->fetchRow());$q++){if(!$q){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>";for($Zf=0;$Zf<count($K);$Zf++){$k=$I->fetchField();if(!$k){echo"<th></th>";continue;}$A=$k->name;$zi=isset($k->orgtable)?$k->orgtable:"";$yi=isset($k->orgname)?$k->orgname:$A;if(isset($k->table))$J[$k->table]=$zi;if($_i&&DIALECT=="sql")$Hg[$Zf]=($A=="table"?"table=":($A=="possible_keys"?"indexes=":null));elseif($zi!=""){if(!isset($t[$zi])){$t[$zi]=[];foreach(indexes($zi,$d)as$s){if($s["type"]=="PRIMARY"){$t[$zi]=array_flip($s["columns"]);break;}}$c[$zi]=$t[$zi];}if(isset($c[$zi][$yi])){unset($c[$zi][$yi]);$t[$zi][$yi]=$Zf;$Hg[$Zf]=$zi;}}if($k->charsetnr==63)$ab[$Zf]=true;$Em[$Zf]=$k->type;$T=trim(($zi!=""?"$zi.$yi":($k->name!=$yi?$yi:""))." ".Driver::get()->getTypeName($k));echo"<th".($T!=""?" title='".h($T)."'":"").">".h($A).($_i?doc_link(['sql'=>"explain-output.html#explain_".strtolower($A),'mariadb'=>"reference/sql-statements/administrative-sql-statements/analyze-and-explain-statements/explain#columns-in-explain-...-select",]):"");}echo"</thead>\n";}echo"<tr>";foreach($K
as$u=>$X){$x="";if(isset($Hg[$u])&&!$c[$Hg[$u]]){if($_i&&DIALECT=="sql"){$Q=$K[array_search("table=",$Hg)];$x=ME.$Hg[$u].urlencode($_i[$Q]!=""?$_i[$Q]:$Q);}else{$x=ME."edit=".urlencode($Hg[$u]);foreach($t[$Hg[$u]]as$Ab=>$Zf)$x
.="&where".urlencode("[".bracket_escape($Ab)."]")."=".urlencode($K[$Zf]);}}$U=($ab[$u]?'blob':($Em[$u]==254?'char':''));$k=['full_type'=>$U,'type'=>$U,];$X=select_value($X,$x,$k,null);$zb=$Em[$u]<=9||$Em[$u]==246?"class='number'":"";echo"<td $zb>$X</td>";}}if($q)echo"</table>\n</div>";else
echo"<p class='message'>".lang(90);echo"\n";return$J;}function
referencable_primary($Fk){$J=[];foreach(table_status('',true)as$Ml=>$Q){if($Ml!=$Fk&&fk_support($Q)){foreach(fields($Ml)as$k){if($k["primary"]){if($J[$Ml]){unset($J[$Ml]);break;}$J[$Ml]=$k;}}}}return$J;}function
textarea($A,$Y,$L=10,$Hb=80){echo"<textarea name='".h($A)."' rows='$L' cols='$Hb' class='sqlarea jush-".DIALECT."' spellcheck='false' wrap='off'>";if(is_array($Y)){foreach($Y
as$X)echo
h($X[0])."\n\n\n";}else
echo
h($Y);echo"</textarea>";}function
select_input($Ma,$C,$Y="",$ii="",$gj=""){if($C&&$Y!=""&&!isset($C[$Y]))$C=[$Y=>$Y]+$C;$Wl=($C?"select":"input");return"<$Wl $Ma".($C?"><option value=''>$gj".optionlist($C,$Y,true)."</select>":" size='10' value='".h($Y)."' placeholder='$gj'>").($ii?script("qsl('$Wl').onchange = $ii;",""):"");}function
json_row($u,$X=null){static$ie=true;if($ie)echo"{";if($u!=""){echo($ie?"":",")."\n\t\"".addcslashes($u,"\r\n\t\"\\/").'": '.($X!==null?'"'.addcslashes($X,"\r\n\t\"\\/").'"':'null');$ie=false;}else{echo"\n}\n";$ie=true;}}function
edit_type($u,$k,$Db,$qe=[],$Rd=[]){$U=isset($k["type"])?$k["type"]:null;echo'<td><select name="',h($u),'[type]" class="type" aria-labelledby="label-type">';$ad=Driver::get()->getTypes();if($U&&!isset($ad[$U])&&!isset($qe[$U])&&!in_array($U,$Rd))$Rd[]=$U;$ul=Driver::get()->getStructuredTypes();if($qe)$ul[lang(91)]=$qe;echo
optionlist(array_merge($Rd,$ul),$U),'</select><td><input name="',h($u),'[length]" value="',h(isset($k["length"])?$k["length"]:null),'" size="3"',(!(isset($k["length"])?$k["length"]:null)&&preg_match('~var(char|binary)$~',$U)?" class='input required'":" class='input'"),' aria-labelledby="label-length"><td class="options">',($Db?"<select name='".h($u)."[collation]'".option_types($U,'(char|text|enum|set)$').'><option value="">('.lang(92).')'.optionlist($Db,isset($k["collation"])?$k["collation"]:null).'</select>':''),(Driver::get()->getUnsigned()?"<select name='".h($u)."[unsigned]'".option_types($U,'^$|'.number_type()).'><option>'.optionlist(Driver::get()->getUnsigned(),isset($k["unsigned"])?$k["unsigned"]:null).'</select>':''),(isset($k['on_update'])?"<select name='".h($u)."[on_update]'".option_types($U,'timestamp|datetime').'>'.optionlist([""=>"(".lang(93).")","CURRENT_TIMESTAMP"],(preg_match('~^CURRENT_TIMESTAMP~i',$k["on_update"])?"CURRENT_TIMESTAMP":$k["on_update"])).'</select>':''),($qe?"<select name='".h($u)."[on_delete]'".option_types($U,'`')."><option value=''>(".lang(94).")".optionlist(Driver::get()->getOnActions(),isset($k["on_delete"])?$k["on_delete"]:null)."</select> ":" ");}function
option_types($U,$Em){return" data-types='".h($Em)."'".(preg_match("~$Em~",$U)?"":" class='hidden'");}function
process_length($v){if(preg_match('~^\s*\(?\s*max\s*\)?\s*$~i',$v))return"(max)";$xd=Driver::$EnumLengthPattern;return(preg_match("~^\\s*\\(?\\s*$xd(?:\\s*,\\s*$xd)*+\\s*\\)?\\s*\$~",$v)&&preg_match_all("~$xd~",$v,$_)?"(".implode(",",$_[0]).")":preg_replace('~^[-+.0-9].*~','(\0)',preg_replace('~[^-0-9,.+()[\]]~','',$v)));}function
process_type($k,$Bb="COLLATE"){return" $k[type]".process_length($k["length"]).(preg_match(number_type(),$k["type"])&&in_array($k["unsigned"],Driver::get()->getUnsigned())?" $k[unsigned]":"").(preg_match('~char|text|enum|set~',$k["type"])&&$k["collation"]?" $Bb ".(DIALECT=="mssql"?$k["collation"]:q($k["collation"])):"");}function
process_field($k,$Bm){if($k["on_update"])$k["on_update"]=preg_replace('~current_timestamp(\(\))?~i',"CURRENT_TIMESTAMP",$k["on_update"]);return[idf_escape(trim($k["field"])),process_type($Bm),($k["null"]?" NULL":" NOT NULL"),default_value($k),(preg_match('~timestamp|datetime~',$k["type"])&&$k["on_update"]?" ON UPDATE ".$k["on_update"]:""),(support("comment")&&$k["comment"]!=""?" COMMENT ".q(normalize_newlines($k["comment"])):""),($k["auto_increment"]?auto_increment():null),];}function
normalize_newlines($Y){return
str_replace("\r","",(string)$Y);}function
default_value($k){if($k["default"]===null)return"";$i=normalize_newlines($k["default"]);$De=$k["generated"];if(in_array($De,Driver::get()->getGenerated())){if(DIALECT=="mssql")return" AS ($i)".($De=="VIRTUAL"?"":" $De");else
return" GENERATED ALWAYS AS ($i) $De";}if(stripos($i,"GENERATED ")===0)return" $i";if(preg_match('~char|binary|text|json|enum|set~',$k["type"])||preg_match('~^(?![a-z])~i',$i)){if(DIALECT=="sql"&&preg_match('~text|json~',$k["type"]))return" DEFAULT (".q($i).")";else
return" DEFAULT ".q($i);}else{$i=str_ireplace("current_timestamp()","CURRENT_TIMESTAMP",$i);return" DEFAULT ".(DIALECT=="sqlite"?"($i)":$i);}}function
type_class($U){foreach(['char'=>'text','date'=>'time|year','binary'=>'blob','enum'=>'set',]as$zb=>$cj){if(preg_match("~$zb|$cj~",$U))return"class='$zb'";}return"";}function
edit_fields(array$l,array$Db,$U="TABLE",$qe=[]){$l=array_values($l);$Pb=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Nb=$Pb?"":"class='hidden'";echo"<thead><tr>\n";if(support("move_col"))echo"<th class='jsonly'></th>";if($U=="PROCEDURE")echo"<td></td>";echo"<th id='label-name'>",($U=="TABLE"?lang(95):lang(96)),"</th>\n","<td id='label-type'>",lang(45),"<textarea id='enum-edit' rows='4' cols='12' wrap='off' hidden></textarea>",script("gid('enum-edit').onblur = onFieldLengthBlur;"),"</td>\n","<td id='label-length'>",lang(97),"</td>\n","<td>",lang(98),"</td>\n";if($U=="TABLE")echo"<td id='label-null'>NULL</td>\n","<td><input type='radio' name='auto_increment_col' value=''><abbr id='label-ai' title='",lang(49),"'>AI</abbr>",doc_link(['sql'=>"example-auto-increment.html",'mariadb'=>"reference/data-types/auto_increment",]),"</td>\n","<td id='label-default'>",lang(99),"</td>\n",support("comment")?"<td id='label-comment' $Nb>".lang(48)."</td>\n":"";echo"<td>","<button name='add[",(support("move_col")?0:count($l)),"]' value='1' title='",lang(100),"' class='button light'>",icon_solo("add"),"</button>",(support("move_col")?"":script("qsl('button').onclick = onAddLastFieldRowClick;")),script("row_count = ".count($l).";"),"</td>\n","</tr></thead>\n";$zb=support("move_col")?"class='sortable'":"";echo"<tbody $zb>\n";foreach($l
as$q=>$k){$q++;$Ai=$k[($_POST?"orig":"field")];$Qc=(isset($_POST["add"][$q-1])||(isset($k["field"])&&!(isset($_POST["drop_col"][$q])?$_POST["drop_col"][$q]:null)))&&(support("drop_col")||$Ai=="");echo"<tr",($Qc?"":" hidden"),">\n";if(support("move_col"))echo"<th class='handle jsonly'>",icon_solo("handle"),"</th>";if($U=="PROCEDURE")echo"<td>",html_select("fields[$q][inout]",Driver::get()->getInOut(),$k["inout"]),"</td>\n";echo"<th>";if($Qc)echo"<input class='input' name='fields[$q][field]' value='",h($k["field"]),"' data-maxlength='64' autocapitalize='off' aria-labelledby='label-name' ".(isset($_POST["add"][$q-1])?"autofocus":"").">";echo
input_hidden("fields[$q][orig]",$Ai);edit_type("fields[$q]",$k,$Db,$qe);echo"</th>\n";if($U=="TABLE"){echo"<td>",checkbox("fields[$q][null]",1,$k["null"],"","","block","label-null"),"</td>\n";$ub=$k["auto_increment"]?"checked":"";echo"<td><label class='block'><input type='radio' name='auto_increment_col' value='$q' $ub aria-labelledby='label-ai'></label></td>\n","<td class='default-value'>";if(Driver::get()->getGenerated())echo
html_select("fields[$q][generated]",array_merge(["","DEFAULT"],Driver::get()->getGenerated()),$k["generated"]);else
echo
checkbox("fields[$q][generated]",1,$k["generated"],"","","","label-default");$Ma="name='fields[$q][default]' aria-labelledby='label-default'";$Y=h($k["default"]);if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";if(support("comment")){$Wg=Connection::get()->isMinVersion("5.5")?1024:255;$Ma="name='fields[$q][comment]' data-maxlength='$Wg' aria-labelledby='label-comment'";$Y=h($k["comment"]);echo"<td $Nb>";if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";}}echo"<td>";if(support("move_col"))echo"<button name='add[$q]' value='1' title='".lang(100)."' class='button light'>",icon_solo("add"),"</button>","<button name='up[$q]' value='1' title='".lang(101)."' class='button light hidden'>",icon_solo("arrow-up"),"</button>","<button name='down[$q]' value='1' title='".lang(102)."' class='button light hidden'>",icon_solo("arrow-down"),"</button>";if($Ai==""||support("drop_col"))echo"<button name='drop_col[$q]' value='1' title='".lang(59)."' class='button light'>",icon_solo("remove"),"</button>";echo"</td>\n</tr>\n";}echo"</tbody>";}function
process_fields(&$l){$Xh=0;if($_POST["up"]){$sg=0;foreach($l
as$u=>$k){if(key($_POST["up"])==$u){unset($l[$u]);array_splice($l,$sg,0,[$k]);break;}if(isset($k["field"]))$sg=$Xh;$Xh++;}}elseif($_POST["down"]){$we=false;foreach($l
as$u=>$k){if(isset($k["field"])&&$we){unset($l[key($_POST["down"])]);array_splice($l,$Xh,0,[$we]);break;}if(key($_POST["down"])==$u)$we=$k;$Xh++;}}elseif($_POST["add"]){$l=array_values($l);array_splice($l,key($_POST["add"]),0,[[]]);}elseif(!$_POST["drop_col"])return
false;return
true;}function
normalize_enum($z){$X=$z[0];return"'".str_replace("'","''",addcslashes(stripcslashes(str_replace($X[0].$X[0],$X[0],substr($X,1,-1))),'\\'))."'";}function
grant($He,array$_j,$c,$gi,$Ym){if(!$_j)return
true;if($_j==["ALL PRIVILEGES","GRANT OPTION"]){if($He)return(bool)queries("GRANT ALL PRIVILEGES ON $gi TO $Ym WITH GRANT OPTION");else
return
queries("REVOKE ALL PRIVILEGES ON $gi FROM $Ym")&&queries("REVOKE GRANT OPTION ON $gi FROM $Ym");}if($_j==["GRANT OPTION","PROXY"]){if($He)return(bool)queries("GRANT PROXY ON $gi TO $Ym WITH GRANT OPTION");else
return(bool)queries("REVOKE PROXY ON $gi FROM $Ym");}return(bool)queries(($He?"GRANT ":"REVOKE ").preg_replace('~(GRANT OPTION)\([^)]*\)~','$1',implode("$c, ",$_j).$c)." ON $gi ".($He?"TO ":"FROM ").$Ym);}function
drop_create($cd,$e,$dd,$bm,$ed,$y,$kh,$ih,$jh,$ei,$Lh){if($_POST["drop"])query_redirect($cd,$y,$kh);elseif($ei=="")query_redirect($e,$y,$jh);elseif(support("transaction_ddl")){Driver::get()->begin();queries_redirect($y,$ih,queries($cd)&&queries($e)&&Driver::get()->commit());Driver::get()->rollback();}elseif($ei!=$Lh){$jc=queries($e);queries_redirect($y,$ih,$jc&&queries($cd));if($jc)queries($dd);}else
queries_redirect($y,$ih,queries($bm)&&queries($ed)&&queries($cd)&&queries($e));}function
create_trigger($gi,array$xm){$km=" $xm[Timing] $xm[Event]".(preg_match('~ OF~',$xm["Event"])?" $xm[Of]":"");return"CREATE TRIGGER ".idf_escape($xm["Trigger"]).(DIALECT=="mssql"?$gi.$km:$km.$gi).rtrim(" $xm[Type]\n$xm[Statement]",";").";";}function
create_routine($ik,$K){$Sk=[];$l=(array)$K["fields"];ksort($l);$rf=implode("|",Driver::get()->getInOut());foreach($l
as$k){if($k["field"]!="")$Sk[]=(preg_match("~^($rf)\$~",$k["inout"])?"$k[inout] ":"").idf_escape($k["field"]).process_type($k,"CHARACTER SET");}$Cc=rtrim($K["definition"],";");return"CREATE $ik ".idf_escape(trim($K["name"]))." (".implode(", ",$Sk).")".($ik=="FUNCTION"?" RETURNS".process_type($K["returns"],"CHARACTER SET"):"").($K["language"]?" LANGUAGE $K[language]":"").(DIALECT=="pgsql"?" AS ".q($Cc):"\n$Cc;");}function
remove_definer($H){return
preg_replace('~^([A-Z =]+) DEFINER=`'.preg_replace('~@(.*)~','`@`(%|\1)',logged_user()).'`~','\1',$H);}function
format_foreign_key($o){$hi=implode("|",Driver::get()->getOnActions());$h=$o["db"];$Rh=$o["ns"];return" FOREIGN KEY (".implode(", ",array_map('AdminNeo\idf_escape',$o["source"])).") REFERENCES ".($h!=""&&$h!=$_GET["db"]?idf_escape($h).".":"").($Rh!=""&&$Rh!=$_GET["ns"]?idf_escape($Rh).".":"").idf_escape($o["table"])." (".implode(", ",array_map('AdminNeo\idf_escape',$o["target"])).")".(preg_match("~^($hi)\$~",$o["on_delete"])?" ON DELETE $o[on_delete]":"").(preg_match("~^($hi)\$~",$o["on_update"])?" ON UPDATE $o[on_update]":"").(isset($o["deferrable"])?" $o[deferrable]":"");}function
tar_file($n,TmpFile$om){$We=pack("a100a8a8a8a12a12",$n,644,0,0,decoct($om->getSize()),decoct(time()));$wb=8*32;for($q=0;$q<strlen($We);$q++)$wb+=ord($We[$q]);$We
.=sprintf("%06o",$wb)."\0 ";echo$We,str_repeat("\0",512-strlen($We));$om->send();echo
str_repeat("\0",511-($om->getSize()+511)%512);}function
doc_link(array$bj,$cm="<sup>?</sup>"){if(!(isset($bj[DIALECT])?$bj[DIALECT]:null))return"";$in=doc_version();$Tm=['sql'=>"https://dev.mysql.com/doc/refman/$in/en/",'sqlite'=>"https://www.sqlite.org/",'pgsql'=>"https://www.postgresql.org/docs/".(Connection::get()->isCockroachDB()?"current":$in)."/",'mssql'=>"https://learn.microsoft.com/en-us/sql/",'oracle'=>"https://www.oracle.com/pls/topic/lookup?ctx=db".str_replace(".","",$in)."&id=",'elastic'=>"https://www.elastic.co/guide/en/elasticsearch/reference/$in/",];if(Connection::get()->isMariaDB()){$Tm['sql']="https://mariadb.com/docs/server/";$bj['sql']=isset($bj['mariadb'])?$bj['mariadb']:str_replace(".html","",$bj['sql']);}return"<a href='".h($Tm[DIALECT].$bj[DIALECT].(DIALECT=='mssql'?"?view=sql-server-ver$in":""))."'".target_blank().">$cm</a>";}function
doc_version(){return
preg_replace('~^(\d\.?\d).*~s','\1',Connection::get()->getVersion());}function
db_size($h){if(!Connection::get()->selectDatabase($h))return"?";$J=0;foreach(table_status()as$R)$J+=$R["Data_length"]+$R["Index_length"];return
format_number($J);}function
set_utf8mb4($e){static$Sk=false;if(!$Sk&&preg_match('~\butf8mb4~i',$e)){$Sk=true;echo"SET NAMES ".charset(Connection::get()).";\n\n";}}error_reporting(E_ALL&~E_DEPRECATED);set_error_handler(function($zd,$j){return(bool)preg_match('~^Undefined (array key|offset|index)~',$j);},E_WARNING|E_NOTICE);;$fe=!preg_match('~^(unsafe_raw)?$~',ini_get("filter.default"));if($fe||ini_get("filter.default_flags")){foreach(['_GET','_POST','_COOKIE','_SERVER']as$X){$Lm=filter_input_array(constant("INPUT$X"),FILTER_UNSAFE_RAW);if($Lm)$$X=$Lm;}}if(function_exists("mb_internal_encoding"))mb_internal_encoding("8bit");class
Server{private$params;private$key;function
__construct(array$Ni,$u=null){$this->params=$Ni;$this->key=$u;}function
getKey(){return
isset($this->key)?$this->key:substr(md5($this->getDriver().$this->getServer()),0,8);}function
getDriver(){return$this->params["driver"];}function
getServer(){return
isset($this->params["server"])?$this->params["server"]:"";}function
getDatabase(){return
isset($this->params["database"])?$this->params["database"]:"";}function
getName(){return
isset($this->params["name"])?$this->params["name"]:(isset($this->params["server"])?$this->params["server"]:"");}function
getUsername(){return
isset($this->params["username"])?$this->params["username"]:"";}function
getPassword(){return
isset($this->params["password"])?$this->params["password"]:"";}function
hasCredentials(){return$this->getUsername()!=""||$this->getPassword()!="";}function
getConfigParams(){$Ni=isset($this->params["config"])?$this->params["config"]:[];$Ge=["servers"];foreach($Ge
as$Mi){if(isset($Ni[$Mi]))unset($Ni[$Mi]);}return$Ni;}}class
Config{static$NavigationSimple="simple";static$NavigationDual="dual";static$NavigationHover="hover";static$NavigationReversed="reversed";private$params;private$servers=[];function
__construct(array$Ni){$this->params=$Ni;if(isset($this->params["servers"])){foreach($this->params["servers"]as$u=>$N){$Kk=new
Server($N,is_string($u)?$u:null);$this->params["servers"][$u]=$Kk;$this->servers[$Kk->getKey()]=$Kk;}}}function
getTheme(){return
isset($this->params["theme"])?$this->params["theme"]:"default";}function
getColorVariant(){return
isset($this->params["colorVariant"])?$this->params["colorVariant"]:"blue";}function
getCssUrls(){return$this->parseList(isset($this->params["cssUrls"])?$this->params["cssUrls"]:[]);}function
getJsUrls(){return$this->parseList(isset($this->params["jsUrls"])?$this->params["jsUrls"]:[]);}function
getNavigationMode(){return
isset($this->params["navigationMode"])?$this->params["navigationMode"]:self::$NavigationSimple;}function
isNavigationSimple(){return$this->getNavigationMode()==self::$NavigationSimple;}function
isNavigationDual(){return$this->getNavigationMode()==self::$NavigationDual;}function
isNavigationReversed(){return$this->getNavigationMode()==self::$NavigationReversed;}function
isSelectionPreferred(){return
isset($this->params["preferSelection"])?$this->params["preferSelection"]:false;}function
isJsonValuesDetection(){return
isset($this->params["jsonValuesDetection"])?$this->params["jsonValuesDetection"]:false;}function
isJsonValuesAutoFormat(){return
isset($this->params["jsonValuesAutoFormat"])?$this->params["jsonValuesAutoFormat"]:false;}function
isRelationLinks(){return
isset($this->params["relationLinks"])?$this->params["relationLinks"]:false;}function
getRecordsPerPage(){return(int)(isset($this->params["recordsPerPage"])?$this->params["recordsPerPage"]:50);}function
getEnumAsSelectThreshold(){if(array_key_exists("enumAsSelectThreshold",$this->params))return$this->params["enumAsSelectThreshold"]!==null?(int)$this->params["enumAsSelectThreshold"]:null;else
return
5;}function
isVersionVerificationEnabled(){return
isset($this->params["versionVerification"])?$this->params["versionVerification"]:true;}function
isSqlAutocompletionEnabled(){return
isset($this->params["sqlAutocompletion"])?$this->params["sqlAutocompletion"]:true;}function
getHiddenDatabases(){return$this->parseList(isset($this->params["hiddenDatabases"])?$this->params["hiddenDatabases"]:[]);}function
getHiddenSchemas(){return$this->parseList(isset($this->params["hiddenSchemas"])?$this->params["hiddenSchemas"]:[]);}function
getVisibleCollations(){return$this->parseList(isset($this->params["visibleCollations"])?$this->params["visibleCollations"]:[]);}function
getDefaultDriver(array$bd){$Zc=isset($this->params["defaultDriver"])?$this->params["defaultDriver"]:null;return$Zc&&isset($bd[$Zc])?$Zc:key($bd);}function
getDefaultServer(){$N=isset($this->params["defaultServer"])?$this->params["defaultServer"]:null;if($N===null)return
null;$Kk=isset($this->params["servers"][$N])?$this->params["servers"][$N]:null;if($Kk)return$Kk->getKey();return$N;}function
getDefaultDatabase(){return
isset($this->params["defaultDatabase"])?$this->params["defaultDatabase"]:null;}function
getDefaultPasswordHash(){return
isset($this->params["defaultPasswordHash"])?$this->params["defaultPasswordHash"]:null;}function
getSslKey(){return
isset($this->params["sslKey"])?$this->params["sslKey"]:null;}function
getSslCertificate(){return
isset($this->params["sslCertificate"])?$this->params["sslCertificate"]:null;}function
getSslCaCertificate(){return
isset($this->params["sslCaCertificate"])?$this->params["sslCaCertificate"]:null;}function
getSslTrustServerCertificate(){return
isset($this->params["sslTrustServerCertificate"])?$this->params["sslTrustServerCertificate"]:null;}function
getSslEncrypt(){return
isset($this->params["sslEncrypt"])?$this->params["sslEncrypt"]:null;}function
getSslMode(){return
isset($this->params["sslMode"])?$this->params["sslMode"]:null;}function
hasServers(){return
isset($this->params["servers"]);}function
getServerPairs(array$bd){$Yk=null;foreach($this->servers
as$N){if(!isset($bd[$N->getDriver()]))continue;if(!$Yk)$Yk=$N->getDriver();elseif($N->getDriver()!=$Yk){$Yk=null;break;}}$Lk=[];foreach($this->servers
as$u=>$N){if(!isset($bd[$N->getDriver()]))continue;$Jk=$N->getName();if($Yk&&$Jk)$Lk[$u]=$Jk;else$Lk[$u]=$bd[$N->getDriver()].($Jk!=""?" - $Jk":"");}return$Lk;}function
getServer($Ik){return
isset($this->servers[$Ik])?$this->servers[$Ik]:null;}function
applyServer($N){$N=$this->getServer($N);if(!$N)return;$this->params=array_merge($this->params,$N->getConfigParams());}private
function
parseList($Jg){if(is_array($Jg))return$Jg;return
preg_split('~\s*,\s*~',(string)$Jg);}}class
Settings{private
static$CookieName="neo_settings";static$ColorSchemeLight="light";static$ColorSchemeDark="dark";static$NavigationWidthMin=10;static$NavigationWidthMax=30;private$config;private$params=[];function
__construct(Config$Tb){$this->config=$Tb;if(isset($_COOKIE[self::$CookieName])){parse_str($_COOKIE[self::$CookieName],$this->params);$this->save();}if(isset($_COOKIE["neo_lang"])){$this->updateParameter("lang",$_COOKIE["neo_lang"]);unset($_COOKIE["neo_lang"]);cookie("neo_lang","",-3600);}}static
function
readParameter($u){parse_str(isset($_COOKIE[self::$CookieName])?$_COOKIE[self::$CookieName]:"",$Ni);return
isset($Ni[$u])?$Ni[$u]:null;}function
getParameter($u,$i=null){return
isset($this->params[$u])?$this->params[$u]:$i;}function
updateParameter($u,$Y){$this->updateParameters([$u=>$Y]);}function
updateParameters(array$Ni){$this->params=array_filter(array_merge($this->params,$Ni),function($Y){return$Y!==null;});$this->save();}private
function
save(){cookie(self::$CookieName,http_build_query($this->params),7776000);}function
getTheme(){return($ra=$this->getParameter("theme"))!==null?$ra:$this->config->getTheme();}function
getColorScheme(){return$this->getParameter("colorScheme");}function
getNavigationMode(){return($ra=$this->getParameter("navigationMode"))!==null?$ra:$this->config->getNavigationMode();}function
isNavigationSimple(){return$this->getNavigationMode()==Config::$NavigationSimple;}function
isNavigationDual(){return$this->getNavigationMode()==Config::$NavigationDual;}function
isNavigationHover(){return$this->getNavigationMode()==Config::$NavigationHover;}function
isNavigationReversed(){return$this->getNavigationMode()==Config::$NavigationReversed;}function
getNavigationWidth(){$_n=$this->getParameter("navigationWidth");if($_n===null)return
null;return
min(max((float)$_n,self::$NavigationWidthMin),self::$NavigationWidthMax);}function
isSelectionPreferred(){return($ra=$this->getParameter("preferSelection"))!==null?$ra:$this->config->isSelectionPreferred();}function
isRelationLinks(){return
isset($this->params["relationLinks"])?$this->params["relationLinks"]:$this->config->isRelationLinks();}function
getRecordsPerPage(){return($ra=$this->getParameter("recordsPerPage"))!==null?$ra:$this->config->getRecordsPerPage();}function
getEnumAsSelectThreshold(){$Y=$this->getParameter("enumAsSelectThreshold");if($Y<0)return
null;return$Y!==null?(int)$Y:$this->config->getEnumAsSelectThreshold();}}class
Hash{static
function
hkdf($v,$u,$xf="",$nk=""){if(extension_loaded("hash")&&PHP_VERSION_ID>=70120)return
hash_hkdf("sha1",$u,$v,$xf,$nk);if($nk=="")$nk=str_repeat("\0",20);$Aj=self::hmacSha1($u,$nk);$bi="";for($gg="",$bb=1;!isset($bi[$v-1]);$bb++){$gg=self::hmacSha1($gg.$xf.chr($bb),$Aj);$bi
.=$gg;}return
substr($bi,0,$v);}static
function
hmacSha1($f,$u){if(!extension_loaded("hash"))return
hash_hmac("sha1",$f,$u,true);if(strlen($u)>64)$u=sha1($u,true);$u=str_pad($u,64,"\0");$Nf=($u^str_repeat("\x36",64));$li=($u^str_repeat("\x5C",64));return
sha1($li.sha1($Nf.$f,true),true);}}class
Random{static
function
strongKey(){return
strtr(rtrim(base64_encode(Random::bytes(32)),"="),"+/","-_");}static
function
bytes($v){if(PHP_VERSION_ID>=70000)return
random_bytes($v);$I=self::tryAlternatives($v);if($I!==false)return$I;$I=self::lastResortRandom($v);if($I!==false)return$I;throw
new
Exception("Error generating random bytes");}private
static
function
tryAlternatives($v){if(extension_loaded("libsodium"))return
\Sodium\randombytes_buf($v);$Km=DIRECTORY_SEPARATOR==="/";if($Km){$I=self::readDevUrandom($v);if($I!==false)return$I;}$fb=$Km&&PHP_VERSION_ID>50609&&PHP_VERSION_ID<50613;if(extension_loaded("mcrypt")&&!$fb){$I=mcrypt_create_iv($v,MCRYPT_DEV_URANDOM);if($I!==false)return$I;}$gb=PHP_VERSION_ID<50444||(PHP_VERSION_ID>50500&&PHP_VERSION_ID<50528)||(PHP_VERSION_ID>50600&&PHP_VERSION_ID<50612);if(extension_loaded("openssl")&&!$gb){$I=openssl_random_pseudo_bytes($v,$sl);if($sl)return$I;}return
false;}private
static
function
readDevUrandom($v){static$m=null;if($m===null)$m=@fopen("/dev/urandom","rb");if(!$m)return
false;$Xj=$v;$I="";do{$f=fread($m,$Xj);if($f===false)return
false;$Xj-=strlen($f);$I
.=$f;}while($Xj>0);return$I;}private
static
function
readCapicom($v){$Jb=new
\COM("CAPICOM.Utilities.1");$Xj=$v;$I="";do{$f=base64_decode((string)$Jb->GetRandom($v,0));$Xj-=strlen($f);$I
.=$f;}while($Xj>0);return$I;}private
static
function
lastResortRandom($v){static$u=null;static$nk=null;if($u===null){$f=$_SERVER;$f[]=uniqid("",true);shuffle($f);$u=sha1(serialize($f),true);if(extension_loaded("openssl"))$nk=openssl_random_pseudo_bytes(20);else{$nk="";for($q=0;$q<20;$q++)$nk
.=chr((mt_rand()^mt_rand())%256);}}else{if((ord($u)%2===0)===(ord($nk)%2===0))$u=Hash::hmacSha1($u,$nk);else$nk=Hash::hmacSha1($nk,$u);}return
Hash::hkdf($v,$u,"$v",$nk);}}if(!function_exists("str_starts_with")){function
str_starts_with($Ve,$Hh){return
strpos($Ve,$Hh)===0;}}if(!function_exists("str_contains")){function
str_contains($Ve,$Hh){return
strpos($Ve,$Hh)!==false;}}if(!function_exists("password_verify")){function
password_verify($F,$Ue){return
false;}}if(!function_exists("ini_set")){function
ini_set($ri,$Y){return
false;}}function
version(){return
VERSION;}function
idf_unescape($nf){if(!preg_match('~^[`\'"[]~',$nf))return$nf;$sg=substr($nf,-1);return
str_replace($sg.$sg,$sg,substr($nf,1,-1));}function
q($P){return
Connection::get()->quote($P);}function
number($X){return
preg_replace('~[^0-9]+~','',$X);}function
int_type(){return'(tiny|small|medium|big)?int(eger|\d)?';}function
number_type(){return'(^('.int_type().'|decimal|numeric|real|(binary_|half_|scaled_)?float\d?|(binary_)?double( precision)?|(small)?money)$)';}function
remove_slashes(array$fn,$fe=false){$J=[];foreach($fn
as$u=>$X)$J[stripslashes($u)]=(is_array($X)?remove_slashes($X,$fe):($fe?$X:stripslashes($X)));return$J;}function
bracket_escape($nf,$Ta=false){static$um=[':'=>':1',']'=>':2','['=>':3','"'=>':4'];return
strtr($nf,($Ta?array_flip($um):$um));}function
min_version($in,$Rg=null,$d=null){if(!$d)$d=Connection::get();if($Rg&&$d->isMariaDB())$in=$Rg;return$in&&$d->isMinVersion($in);}function
charset(Connection$d){return($d->isMinVersion("5.5.3")?"utf8mb4":"utf8");}function
link_files($A,array$ee){switch($A){case'favicon-blue.ico':$n='favicon-blue-0f5ce53a66b1e25395d0048da369f19e__519803fd.ico';break;case'favicon-blue.svg':$n='favicon-blue-17e440832c1eac07527560a0d6f0d2ee__519803fd.svg';break;case'apple-touch-icon-blue.png':$n='apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__519803fd.png';break;case'logo.svg':$n='logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg';break;case'jush.css':$n='jush-b3a93b18444da26820ff61746521dede__21b065eb.css';break;case'jush-dark.css':$n='jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css';break;case'jush.js':$n='jush-615bc0b9720a1de8edd2c6876a3495b6__12a85d92.js';break;case'icons.svg':$n='icons-70163a2695280bf75edba563e7b5471b__273d3fbc.svg';break;case'default-blue.css':$n='default-blue-e7acfdb81453b86f081569afa115859e__b628fd70.css';break;case'default-blue-dark.css':$n='default-blue-dark-1061ad7d216f143e3626560b92b66061__21b065eb.css';break;case'main.js':$n='main-c1727fb392fab162b6611efe18642a5a__e1a04d38.js';break;default:$n=null;break;}if(!$n)return
null;return
BASE_URL."?file=".urldecode($n);}function
ini_bool($ri){$X=ini_get($ri);return
preg_match('~^(on|true|yes)$~i',$X)||(int)$X;}function
ini_bytes($zf){$X=ini_get($zf);switch(strtolower(substr($X,-1))){case'g':$X=(int)$X*1024;case'm':$X=(int)$X*1024;case'k':$X=(int)$X*1024;}return$X;}function
max_input_vars($K,$Gi){$Tg=(int)ini_get("max_input_vars");return($Tg?(int)floor(($Tg-$Gi)/$K):0);}function
max_input_vars_error(){$zf="max_input_vars";return
lang(103,"$zf = ".(int)ini_get($zf));}function
sid(){static$J;if($J===null)$J=(session_id()&&!($_COOKIE&&ini_bool("session.use_cookies")));return$J;}function
save_driver_name($Zc,$N,$A){restart_session();$_SESSION["drivers"][$Zc][$N]=$A;stop_session();}function
get_driver_name($Zc,$N=null){return
isset($_SESSION["drivers"][$Zc][$N])?$_SESSION["drivers"][$Zc][$N]:Drivers::get($Zc);}function
save_login($Zc,$N,$V,$F,$h=""){$u=isset($_COOKIE["neo_key"])?$_COOKIE["neo_key"]:null;$_SESSION["pwds"][$Zc][$N][$V]=$u?[encrypt_string($F,$u)]:$F;$_SESSION["db"][$Zc][$N][$V][$h]=true;}function
delete_login($Zc,$N,$V){unset($_SESSION["pwds"][$Zc][$N][$V]);unset($_SESSION["db"][$Zc][$N][$V]);}function
get_password(){$F=get_session("pwds");if(is_array($F))return$_COOKIE["neo_key"]?decrypt_string($F[0],$_COOKIE["neo_key"]):false;return$F;}function
get_vals($H,$b=0){$J=[];$I=Connection::get()->query($H);if(is_object($I)){while($K=$I->fetchRow())$J[]=$K[$b];}return$J;}function
get_key_vals($H,$d=null,$Tk=true){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchRow()){if($Tk)$J[$K[0]]=$K[1];else$J[]=$K[0];}}return$J;}function
get_rows($H,$d=null,$j="<p class='error'>"){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchAssoc())$J[]=$K;}elseif(!$I&&!is_object($d)&&$j&&(defined("AdminNeo\PAGE_HEADER")||$j=="-- "))echo$j.error()."\n";return$J;}function
unique_array(array$K,array$t){foreach($t
as$s){if(!preg_match("~^(PRIMARY|UNIQUE)$~",$s["type"])||(isset($s["partial"])?$s["partial"]:"")!="")continue;$Hm=[];foreach($s["columns"]as$u){if(!isset($K[$u]))continue
2;$Hm[$u]=$K[$u];}return$Hm;}return
null;}function
escape_key($u){if(preg_match('(^([\w(]+)('.str_replace("_",".*",preg_quote(idf_escape("_"))).')([ \w)]+)$)',$u,$z))return$z[1].idf_escape(idf_unescape($z[2])).$z[3];return
idf_escape($u);}function
where($Z,$l=[]){$Sb=[];foreach((array)$Z["where"]as$u=>$X){$u=bracket_escape($u,true);$b=escape_key($u);$k=isset($l[$u])?$l[$u]:null;$ae=isset($k["type"])?$k["type"]:null;$_e=isset($k["full_type"])?$k["full_type"]:null;$Rf=$k&&(is_blob($k)||preg_match('~binary~',$ae));if($Rf&&!is_utf8($X))$Sb[]="$b = ".Driver::get()->quoteBinary($X);elseif(DIALECT=="sql"&&$ae=="json")$Sb[]="$b = CAST(".q($X)." AS JSON)";elseif(DIALECT=="pgsql"&&preg_match('~^jsonb?$~',$_e))$Sb[]="$b::jsonb = ".q($X)."::jsonb";elseif(DIALECT=="sql"&&is_numeric($X)&&strpos($X,".")!==false)$Sb[]="$b LIKE ".q($X);elseif(DIALECT=="mssql"&&strpos($ae,"datetime")===false)$Sb[]="$b LIKE ".q(preg_replace('~[_%[]~','[\0]',$X));else$Sb[]="$b = ".(isset($l[$u])?unconvert_field($l[$u],q($X)):q($X));if(DIALECT=="sql"&&preg_match('~char|text~',$ae)&&preg_match("~[^ -@]~",$X))$Sb[]="$b = ".q($X)." COLLATE ".charset(Connection::get())."_bin";}foreach((array)$Z["null"]as$u)$Sb[]=escape_key($u)." IS NULL";return
implode(" AND ",$Sb);}function
where_columns($Z,$l=[]){$c=[];foreach((array)$Z["null"]as$u)$c[$u]=true;foreach((array)$Z["where"]as$u=>$X){$u=bracket_escape($u,true);foreach($l
as$A=>$k){if($u==$A||strpos($u,idf_escape($A))!==false)$c[$A]=true;}}return$c;}function
where_check($X,$l=[]){parse_str($X,$rb);remove_slashes([&$rb]);return
where($rb,$l);}function
where_link($q,$b,$Y,$oi="="){return"&where%5B$q%5D%5Bcol%5D=".urlencode($b)."&where%5B$q%5D%5Bop%5D=".urlencode(($Y!==null?$oi:"IS NULL"))."&where%5B$q%5D%5Bval%5D=".urlencode($Y);}function
convert_fields(array$c,array$l,array$M=[]){$I="";foreach($c
as$u=>$X){if($M&&!in_array(idf_escape($u),$M))continue;$La=convert_field($l[$u]);if($La)$I
.=", $La AS ".idf_escape($u);}return$I;}function
cookie_path(){return
strtr(preg_replace('~\?.*~','',$_SERVER["REQUEST_URI"]),[";"=>"%3B",","=>"%2C"]);}function
cookie($A,$Y,$Bg=2592000){header("Set-Cookie: $A=".rawurlencode($Y).($Bg?"; expires=".gmdate("D, d M Y H:i:s",time()+$Bg)." GMT":"")."; path=".cookie_path().(HTTPS?"; secure":"")."; HttpOnly; SameSite=lax",false);}function
get_url($Sm,$Zb){$http_response_header=null;$Ad=[];set_error_handler(function($zd,$j)use(&$Ad){$Ad[]=preg_replace('~^file_get_contents\([^)]*\):\s*~','',$j);return
true;});$J=file_get_contents($Sm,false,$Zb);restore_error_handler();$Xe=function_exists('http_get_last_response_headers')?http_get_last_response_headers():$http_response_header;return[$J,(array)$Xe,$J===false?implode("\n",$Ad):"",];}function
get_settings($cc="neo_settings"){parse_str(isset($_COOKIE[$cc])?$_COOKIE[$cc]:"",$O);return$O;}function
get_setting($u,$cc="neo_settings"){$O=get_settings($cc);return
isset($O[$u])?$O[$u]:null;}function
save_settings(array$O,$cc="neo_settings"){cookie($cc,http_build_query($O+get_settings($cc)));}function
restart_session(){if(!ini_bool("session.use_cookies")&&session_status()==PHP_SESSION_NONE)session_start();}function
stop_session($ne=false){$Wm=ini_bool("session.use_cookies");if(!$Wm||$ne){session_write_close();if($Wm&&ini_set("session.use_cookies","0")===false)session_start();}}function&get_session($u){return$_SESSION[$u][DRIVER][SERVER][$_GET["username"]];}function
set_session($u,$X){$_SESSION[$u][DRIVER][SERVER][$_GET["username"]]=$X;}function
auth_url($hn,$N,$V,$h=null){$Rm=remove_from_uri(implode("|",array_keys(Drivers::getList()))."|username|ext|".($h!==null?"db|":"").($hn=='mssql'||$hn=='pgsql'?"":"ns|").session_name());preg_match('~([^?]*)\??(.*)~',$Rm,$z);return"$z[1]?".(sid()?session_name()."=".urlencode(session_id())."&":"").($_GET["ext"]?"ext=".urlencode($_GET["ext"])."&":"").urlencode($hn)."=".urlencode($N)."&"."username=".urlencode($V).($h!=""?"&db=".urlencode($h):"").($z[2]?"&$z[2]":"");}function
is_ajax(){return($_SERVER["HTTP_X_REQUESTED_WITH"]=="XMLHttpRequest");}function
redirect($y,$hh=null){if($hh!==null){restart_session();$_SESSION["messages"][preg_replace('~^[^?]*~','',($y!==null?$y:$_SERVER["REQUEST_URI"]))][]=$hh;}if($y!==null){if($y=="")$y=".";header("Location: $y");exit;}}function
query_redirect($H,$y,$hh,$Oj=true,$Gd=true,$Sd=false,$im=""){if($Gd){$ml=microtime(true);$Sd=!Connection::get()->query($H);$im=format_time($ml);}$hl=$H?Admin::get()->formatMessageQuery($H,$im,$Sd):"";if($Sd){Admin::get()->addError(error().$hl.script("initToggles();"));return
false;}if($Oj)redirect($y,$hh.$hl);return
true;}function
queries_redirect($y,$hh,$Oj){$Fj=implode("\n",Queries::$queries);$im=format_time(Queries::$start);return
query_redirect($Fj,$y,$hh,$Oj,false,!$Oj,$im);}class
Queries{static$queries=[];static$start=0.0;}function
remember_query($H){if(!Queries::$start)Queries::$start=microtime(true);if(support("sql"))Queries::$queries[]=(preg_match('~;$~',$H)?"DELIMITER ;;\n$H;\nDELIMITER ":$H).";";else
Queries::$queries[]=$H;}function
queries($H){remember_query($H);return
support("sql")?Connection::get()->query($H):[];}function
apply_queries($H,array$S,$Bd='AdminNeo\table'){foreach($S
as$Q){if(!queries("$H ".$Bd($Q)))return
false;}return
true;}function
format_time($ml){return
lang(104,max(0,microtime(true)-$ml));}function
relative_uri(){return
str_replace(":","%3a",preg_replace('~^[^?]*/([^?]*)~','\1',$_SERVER["REQUEST_URI"]));}function
remove_from_uri($Mi=""){return
substr(preg_replace("~(?<=[?&])($Mi".(sid()?"":"|".session_name()).")=[^&]*&~",'',relative_uri()."&"),0,-1);}function
get_file($u,$yc=false,$Ec=""){$m=$_FILES[$u];if(!$m)return
null;foreach($m
as$u=>$X)$m[$u]=(array)$X;$J='';foreach($m["error"]as$u=>$j){if($j)return$j;$A=$m["name"][$u];$pm=$m["tmp_name"][$u];$Xb=file_get_contents($yc&&preg_match('~\.gz$~',$A)?"compress.zlib://$pm":$pm);if($yc){$ml=substr($Xb,0,3);if(function_exists("iconv")&&preg_match("~^\xFE\xFF|^\xFF\xFE~",$ml))$Xb=iconv("utf-16","utf-8",$Xb);elseif($ml=="\xEF\xBB\xBF")$Xb=substr($Xb,3);}if($Ec){if(!preg_match("~$Ec\\s*\$~",$Xb))$Xb
.=";";$Xb
.="\n\n";}$J
.=$Xb;}return$J;}function
upload_error($j){$bh=($j==UPLOAD_ERR_INI_SIZE?ini_get("upload_max_filesize"):0);return($j?lang(105).($bh?" ".lang(106,$bh):""):lang(107));}function
repeat_pattern($cj,$v){return
str_repeat("$cj{0,65535}",$v/65535)."$cj{0,".($v%65535)."}";}function
is_utf8($X){return(preg_match('~~u',$X)&&!preg_match('~[\0-\x8\xB\xC\xE-\x1F]~',$X));}function
format_number($X){return
strtr(number_format($X,0,".",lang(108)),preg_split('~~u',lang(109),-1,PREG_SPLIT_NO_EMPTY));}function
format_rows(array$R){$L=$R["Rows"];$Ia=($L&&(DIALECT=="sqlite"||(isset($R["Engine"])?$R["Engine"]:"")==(DIALECT=="pgsql"?"table":"InnoDB")));return($Ia?"~ ":"").format_number($L);}function
friendly_url($X){return
preg_replace('~\W~i','-',$X);}function
table_status1($Q,$Ud=false){$J=table_status($Q,$Ud);return($J?reset($J):["Name"=>$Q]);}function
column_foreign_keys($Q){$J=[];foreach(Admin::get()->getForeignKeys($Q)as$o){foreach($o["source"]as$X)$J[$X][]=$o;}return$J;}function
fields_from_edit(){$J=[];foreach((array)$_POST["field_keys"]as$u=>$X){if($X!=""){$X=bracket_escape($X);$_POST["function"][$X]=$_POST["field_funs"][$u];$_POST["fields"][$X]=$_POST["field_vals"][$u];}}foreach((array)$_POST["fields"]as$u=>$X){$A=bracket_escape($u,true);$J[$A]=["field"=>$A,"full_type"=>"varchar","type"=>"varchar","privileges"=>["insert"=>1,"update"=>1,"where"=>1,"order"=>1],"null"=>true,"auto_increment"=>($u==Driver::get()->primary),];}return$J;}function
dump_headers($lf,$zh=false){$lf=friendly_url($lf).date("-Ymd-His");$Od=Admin::get()->sendDumpHeaders($lf,$zh);if($Od===null)$Od=Admin::get()->sendDumpFormatHeaders($lf,$zh);Admin::get()->sendDumpOutputHeaders($lf,$Od);$Ii=$_POST["output"];if($Ii!="text")header("Content-Disposition: attachment; filename=$lf.$Od".($Ii!="file"&&preg_match('~^[0-9a-z]+$~',$Ii)?".$Ii":""));session_write_close();if(!ob_get_level())ob_start(null,4096);ob_flush();flush();return$Od;}function
dump_table_order(array$Fh,array$Uj){$lg=array_flip($Fh);$wi=[];$qn=[];$oc=false;$pn=function($A)use(&$pn,&$wi,&$qn,&$oc,$lg,$Uj){if(isset($wi[$A]))return;if(isset($qn[$A])){$oc=true;return;}$qn[$A]=true;foreach(isset($Uj[$A])?$Uj[$A]:[]as$Sj){if(isset($lg[$Sj]))$pn($Sj);}unset($qn[$A]);$wi[$A]=true;};foreach($Fh
as$A)$pn($A);return($oc?null:array_keys($wi));}function
dump_csv($K){$Am=$_POST["format"]=="tsv";foreach($K
as$u=>$X){if(preg_match('~["\n]|^0[^.]|\.\d*0$|'.($Am?'\t':'[,;]|^$').'~',$X))$K[$u]='"'.str_replace('"','""',$X).'"';}echo
implode(($_POST["format"]=="csv"?",":($Am?"\t":";")),$K)."\r\n";}function
apply_sql_function($p,$b){return($p?($p=="unixepoch"?"DATETIME($b, '$p')":($p=="count distinct"?"COUNT(DISTINCT ":strtoupper("$p("))."$b)"):$b);}function
get_temp_dir(){$aj=ini_get("upload_tmp_dir");if(!$aj)$aj=sys_get_temp_dir();return$aj;}function
open_file_with_lock($n){if(is_link($n))return
null;$m=@fopen($n,"c+");if(!$m)return
null;@chmod($n,0660);if(!flock($m,LOCK_EX)){fclose($m);return
null;}return$m;}function
write_and_unlock_file($m,$f){rewind($m);fwrite($m,$f);ftruncate($m,strlen($f));unlock_file($m);}function
unlock_file($m){flock($m,LOCK_UN);fclose($m);}function
first(array$Ka){return
reset($Ka);}function
get_private_key($e){$n=get_temp_dir()."/adminneo.key";if(!$e&&!file_exists($n))return
false;$m=open_file_with_lock($n);if(!$m)return
false;$u=stream_get_contents($m);if(!$u){$u=Random::strongKey();write_and_unlock_file($m,$u);}else
unlock_file($m);return$u;}function
get_random_string(){return
Random::strongKey();}function
select_value($X,$x,$k,$em){if(is_array($X)){$J="";if(array_filter($X,'is_array')==array_values($X)){$hg=[];foreach($X
as$W)$hg+=array_fill_keys(array_keys($W),null);foreach(array_keys($hg)as$cg)$J
.="<th>".h($cg);foreach($X
as$W){$J
.="<tr>";foreach(array_merge($hg,$W)as$cn)$J
.="<td>".select_value($cn,$x,$k,$em);}}else{foreach($X
as$cg=>$W)$J
.="<tr>".($X!=array_values($X)?"<th>".h($cg):"")."<td>".select_value($W,$x,$k,$em);}return"<table>$J</table>";}$sk="";if($k&&$X!==null&&($em===null||strlen($X)<=$em)&&($fn=Driver::get()->explodeArrayValue($X,$k["full_type"],$sk))){$rk=$k;$rk["type"]=$rk["full_type"]=$sk;$J=select_array_value($fn,$X,$x,$rk,$em);return
Driver::get()->implodeArrayValues($J,$k["full_type"]);}if(!$x)$x=Admin::get()->getFieldValueLink($X,$k);if($k)$X=Connection::get()->formatValue($X,$k);$J=$k?Admin::get()->formatFieldValue($X,$k):$X;if($J!==null){if(!is_utf8($J))$J="\0";elseif($em!=""&&is_shortable($k))$J=truncate_utf8($J,max(0,+$em));else$J=h($J);}return
Admin::get()->formatSelectionValue($J,$x,$k,$X);}function
select_array_value(array$fn,$X,$x,array$k,$em){$I=[];foreach($fn
as$Y){if(is_array($Y))$I[]=select_array_value($Y,$X,$x,$k,$em);else{$mg=preg_replace('~(where%5B\d+%5D%5Bval%5D=)'.preg_quote(urlencode($X),"~")."~",'${1}'.urlencode($Y),$x);$I[]=select_value($Y,$mg,$k,$em);}}return$I;}function
is_blob(array$k){$Em=Driver::get()->getStructuredTypes();$U=lang(110);return
preg_match('~blob|bytea|raw|file'.(DIALECT=="mssql"?'|binary|image':'').'~',$k["type"])&&!in_array($k["type"],isset($Em[$U])?$Em[$U]:[]);}function
is_generated_always(array$k){return(isset($k["generated"])?$k["generated"]:"")!=""||stripos((string)(isset($k["default"])?$k["default"]:""),"GENERATED ALWAYS AS ")===0;}function
parenthesize_expression($Md){$rl=preg_replace('~\'(?:[^\']|\'\')*\'|"(?:[^"]|"")*"|`(?:[^`]|``)*`|\[[^]]*]~',"",$Md);if(!preg_match('~^\(.*\)$~s',$rl))return"($Md)";$Gc=0;$sg=strlen($rl)-1;for($q=0;$q<$sg;$q++){if($rl[$q]=="(")$Gc++;elseif($rl[$q]==")"&&--$Gc==0)return"($Md)";}return$Md;}function
is_mail($Y){return
is_string($Y)&&filter_var($Y,FILTER_VALIDATE_EMAIL);}function
is_web_url($Y){if(!is_string($Y)||!preg_match('~^(https?:)?//~i',$Y))return
false;$Qb=parse_url($Y);if(!$Qb)return
false;$Sm=$Y;if(isset($Qb['path'])){$td=array_map('urlencode',explode('/',$Qb['path']));$Sm=str_replace($Qb['path'],implode('/',$td),$Sm);}if(isset($Qb['query'])){parse_str($Qb['query'],$Ni);$Sm=str_replace($Qb['query'],http_build_query($Ni),$Sm);}if(!isset($Qb['scheme']))$Sm="https:$Sm";return(bool)filter_var($Sm,FILTER_VALIDATE_URL);}function
is_shortable($k){return$k&&!preg_match('~'.number_type().'|date|time|year~',$k["type"]);}function
host_port($N){return(preg_match('~^(:([^:].*)|(\[(.+)]|(([^:]+://)?[^:]+))(:(\d+))?)$~',$N,$z)?[(isset($z[4])?$z[4]:"").(isset($z[5])?$z[5]:""),$z[2].(isset($z[8])?$z[8]:"")]:[$N,'']);}function
count_rows($Q,$Z,$Sf,$Ke){$H=" FROM ".table($Q).($Z?" WHERE ".implode(" AND ",$Z):"");return($Sf&&(DIALECT=="sql"||count($Ke)==1)?"SELECT COUNT(DISTINCT ".implode(", ",$Ke).")$H":"SELECT COUNT(*)".($Sf?" FROM (SELECT 1$H GROUP BY ".implode(", ",$Ke).") x":$H));}function
slow_query($H){$h=Admin::get()->getDatabase();$jm=Admin::get()->getQueryTimeout();$al=Driver::get()->slowQuery($H,$jm);$d=null;if(!$al&&support("kill")){$d=connect();if($d&&($h==""||$d->selectDatabase($h))){$jg=number($d->getValue(connection_id()));echo'<script',nonce(),'>
	const timeout = setTimeout(() => {
		ajax(\'',js_escape(ME),'script=kill\', function() {
		}, \'kill=',$jg,'&token=',get_token(),'\');
	}, ',1000*$jm,');
</script>
';}}ob_flush();flush();$J=@get_key_vals(($al?:$H),$d,false);if($d){echo
script("clearTimeout(timeout);");ob_flush();flush();}return$J;}function
get_token(){$Kj=rand(1,1e6);return($Kj^$_SESSION["token"]).":$Kj";}function
verify_token(){list($qm,$Kj)=explode(":",$_POST["token"]);return($Kj^$_SESSION["token"])==$qm&&in_array($_SERVER["HTTP_SEC_FETCH_SITE"],["","same-origin"]);}function
script($el,$tm="\n"){return"<script".nonce().">$el</script>$tm";}function
script_src($Sm,$Bc=false){return"<script src='".h($Sm)."'".nonce().($Bc?" defer":"")."></script>\n";}function
nonce(){return' nonce="'.get_nonce().'"';}function
input_hidden($A,$Y=""){return"<input type='hidden' name='".h($A)."' value='".h($Y)."'>";}function
input_token(){return
input_hidden("token",get_token());}function
target_blank(){return' target="_blank" rel="noreferrer noopener"';}function
h($P){if($P===null||$P==="")return"";return
str_replace(["&","<","\"","'","\0"],["&amp;","&lt;","&quot;","&#039;","&#0;"],$P);}function
truncate_utf8($P,$v=80){if($P=="")return"";if(!preg_match("(^(".repeat_pattern("[\t\r\n -\x{10FFFF}]",$v).")($)?)u",$P,$z))preg_match("(^(".repeat_pattern("[\t\r\n -~]",$v).")($)?)",$P,$z);return
h($z[1]).(isset($z[2])?"":"<i>…</i>");}function
icon_solo($r){return
icon($r,"solo");}function
icon_chevron_down(){return
icon("chevron-down","chevron");}function
icon_chevron_right(){return
icon("chevron-down","chevron-right");}function
icon($r,$zb=null){$r=h($r);return"<svg class='icon ic-$r $zb'><use href='".link_files("icons.svg",[])."#$r'/></svg>";}function
checkbox($A,$Y,$ub,$ng="",$ji="",$zb="",$pg=""){$J="<input type='checkbox' name='$A' value='".h($Y)."'".($ub?" checked":"").($pg?" aria-labelledby='$pg'":"").">".($ji?script("qsl('input').onclick = function () { $ji };",""):"");return($ng!=""||$zb?"<label".($zb?" class='$zb'":"").">$J".h($ng)."</label>":$J);}function
optionlist($C,$Ck=null,$Xm=false){$J="";foreach($C
as$cg=>$W){$ti=[$cg=>$W];if(is_array($W)){$J
.='<optgroup label="'.h($cg).'">';$ti=$W;}foreach($ti
as$u=>$X)$J
.='<option'.($Xm||is_string($u)?' value="'.h($u).'"':'').($Ck!==null&&($Xm||is_string($u)?(string)$u:$X)===$Ck?' selected':'').'>'.h($X);if(is_array($W))$J
.='</optgroup>';}return$J;}function
html_select($A,$C,$Y="",$ii="",$pg="",$Xm=false){static$ng=0;$og="";if(!$pg&&substr(isset($C[""])?$C[""]:"",0,1)=="("){$ng++;$pg="label-$ng";$og="<option value='' id='$pg'>".h($C[""]);unset($C[""]);}return"<select name='".h($A)."'".($pg?" aria-labelledby='$pg'":"").">".$og.optionlist($C,$Y,$Xm)."</select>".($ii?script("qsl('select').onchange = function () { $ii };",""):"");}function
html_radios($A,$C,$Y=""){$I="<span class='labels'>";foreach($C
as$u=>$X)$I
.="<label><input type='radio' name='".h($A)."' value='".h($u)."'".($u==$Y?" checked":"").">".h($X)."</label>";$I
.="</span>";return$I;}function
confirm($hh="",$Ek="qsl('input')"){return
script("$Ek.onclick = () => confirm('".js_escape($hh?:lang(111))."');","");}function
print_fieldset_start($r,$yg,$kf,$nn=false,$cl=false){echo"<fieldset id='fieldset-$r' class='closable ".(!$nn?" closed":"")."'>","<legend><a href='#'>$yg</a></legend>",icon($kf,"fieldset-icon jsonly"),"<div class='fieldset-content".($cl?" sortable":"")."'>";}function
print_fieldset_end($r,$cl=false){echo"</div>",script("initFieldset('$r');","");if($cl)echo
script("initSortable('#fieldset-$r .fieldset-content');","");echo"</fieldset>\n";}function
bold($cb,$zb=""){return($cb?" class='$zb active'":($zb?" class='$zb'":""));}function
js_escape($P){return
str_replace("<","\\x3C",addcslashes($P,"\r\n'\\"));}function
js_escape_key($P){return'"'.str_replace("<","\\x3C",addcslashes($P,"\r\n\t\"\\")).'"';}function
js_escape_re($P){return
addcslashes(preg_quote($P,"/"),"\r\n");}function
pagination($E,$lc){return"<li>".($E==$lc?"<strong>".($E+1)."</strong>":'<a href="'.h(remove_from_uri("page").($E?"&page=$E".($_GET["next"]?"&next=".urlencode($_GET["next"]):""):"")).'">'.($E+1)."</a>")."</li>";}function
print_hidden_fields(array$Bj,array$of=[],$tj=""){$I=false;foreach($Bj
as$u=>$X){if(!in_array($u,$of)){if(is_array($X))print_hidden_fields($X,[],$u);else{$I=true;echo
input_hidden($tj?$tj."[$u]":$u,$X);}}}return$I;}function
hidden_fields_get(){if(sid())echo
input_hidden(session_name(),session_id());if($_GET["ext"])echo
input_hidden("ext",$_GET["ext"]);if(SERVER!==null)echo
input_hidden(DRIVER,SERVER);echo
input_hidden("username",$_GET["username"]);}function
enum_input($Ma,array$k,$Y,$rd=null,$tb=false){preg_match_all("~'((?:[^']|'')*)'~",$k["length"],$_);$fn=$_[1];$hm=Admin::get()->getSettings()->getEnumAsSelectThreshold();$M=!$tb&&$hm!==null&&count($fn)>$hm;$U=$tb?"checkbox":"radio";$xa=$M?"selected":"checked";$I=$M?"<select $Ma>":"<span class='labels'>";if($M&&$k["null"]&&$rd!==""){$ub=$Y===null?$xa:"";$I
.="<option value='__adminneo_empty__' disabled $ub></option>";}if($rd!==null){$ub=(is_array($Y)?in_array($rd,$Y):$Y===$rd)?$xa:"";if($M)$I
.="<option value='$rd' $ub>".lang(112)."</option>";else$I
.="<label><input type='$U' $Ma value='$rd' $ub><i>".lang(112)."</i></label>";}foreach($fn
as$X){if($rd===""&&$X==="")continue;$X=stripcslashes(str_replace("''","'",$X));$ub=is_array($Y)?in_array($X,$Y):$Y===$X;$ub=$ub?$xa:"";$ve=$X===""?("<i>".lang(112)."</i>"):h(Admin::get()->formatFieldValue($X,$k));if($M)$I
.="<option value='".h($X)."' $ub>$ve</option>";else$I
.=" <label><input type='$U' $Ma value='".h($X)."' $ub>$ve</label>";}$I
.=$M?"</select>":"</span>";return$I;}function
input($k,$Y,$p,$Qa=false){$A=h(bracket_escape($k["field"]));$Em=Driver::get()->getTypes();$Tf=isset($k["full_type"])&&Admin::get()->detectJson($k["full_type"],$Y,true);$Zj=(DIALECT=="mssql"&&$k["auto_increment"]&&!$_POST["clone"]);if($Zj&&!$_POST["save"])$p=null;$Cm=Driver::get()->getUserTypeId(isset($k["type"])?$k["type"]:"");if($Cm){$yd=type_values($Cm);if($yd){$k["type"]="enum";$k["length"]=$yd;}}$Ma=" name='fields[$A]' ".($Qa?" autofocus":"");$Ce=(isset($_GET["select"])||$Zj?["orig"=>lang(113)]:[])+Admin::get()->getFieldFunctions($k);$Te=(in_array($p,$Ce)||isset($Ce[$p]));echo"<td class='function'>",Driver::get()->getUnconvertFunction($k)." ";if(count($Ce)>1){$Ck=$p===null||$Te?$p:"";echo"<select name='function[$A]'>".optionlist($Ce,$Ck)."</select>",help_script_command("value.replace(/^SQL\$/, '')",true),script("qsl('select').onchange = functionChange;","");}else
echo
h(reset($Ce));echo"</td><td>";$_f=Admin::get()->getFieldInput(isset($_GET["edit"])?$_GET["edit"]:null,$k,$Ma,$Y,$p);if($_f!="")echo$_f;elseif(preg_match('~bool~',$k["type"]))echo"<input type='hidden'$Ma value='0'>"."<input type='checkbox'".(preg_match('~^(1|t|true|y|yes|on)$~i',$Y)?" checked":"")."$Ma value='1'>";elseif($k["type"]=="enum")echo
enum_input($Ma,$k,$Y);elseif($k["type"]=="set"){preg_match_all("~'((?:[^']|'')*)'~",$k["length"],$_);echo"<span class='labels'>";foreach($_[1]as$X){$X=stripcslashes(str_replace("''","'",$X));$ub=$Y!==null&&in_array($X,explode(",",$Y),true);$ub=$ub?"checked":"";$ve=$X===""?("<i>".lang(112)."</i>"):h(Admin::get()->formatFieldValue($X,$k));echo" <label><input type='checkbox' name='fields[$A][]' value='".h($X)."' $ub>$ve</label>";}echo"</span>";}elseif(is_blob($k)&&ini_bool("file_uploads"))echo"<input type='file' name='fields-$A'>";elseif($Tf)echo"<textarea $Ma cols='50' rows='12' class='jush-json'>".h($Y).'</textarea>';elseif(($cm=preg_match('~text|lob|memo|json~i',$k["type"])||(preg_match('~char|binary~',$k["type"])&&$k["length"]=="max"))||preg_match("~\n~",$Y)){if($cm&&DIALECT!="sqlite")$Ma
.=" cols='50' rows='12'";else{$L=min(12,substr_count($Y,"\n")+1);$Ma
.=" cols='30' rows='$L'";}echo"<textarea $Ma>".h($Y).'</textarea>';}else{$dh=!preg_match('~int~',$k["type"])&&preg_match('~^(\d+)(,(\d+))?$~',$k["length"],$z)?((preg_match("~binary~",$k["type"])?2:1)*$z[1]+($z[3]?1:0)+($z[2]&&!$k["unsigned"]?1:0)):($Em&&$Em[$k["type"]]?$Em[$k["type"]]+($k["unsigned"]?0:1):0);if(DIALECT=='sql'&&Connection::get()->isMinVersion("5.6")&&preg_match('~time~',$k["type"]))$dh+=7;echo"<input class='input'".((!$Te||$p==="")&&preg_match('~^'.int_type().'$~',$k["type"])&&!preg_match('~\[]~',$k["full_type"])?" type='number'":"").($p!="now"?" value='".h($Y)."'":" data-last-value='".h($Y)."'").($dh?" data-maxlength='$dh'":"").(preg_match('~char|binary~',$k["type"])&&$dh>20?" size='44'":"")."$Ma>";}$bf=Admin::get()->getFieldInputHint($_GET["edit"],$k,$Y);if($bf!="")echo" <span class='input-hint'>$bf</span>";if(count($Ce)>1)echo
script("qs('select', qsl('td').previousSibling).onchange(null, true);","");$je=0;foreach($Ce
as$u=>$X){if($u===""||!$X)break;$je++;}if(count($Ce)>1)echo
script("qsl('td').oninput = partial(skipOriginal, $je);");}function
process_input($k){if(is_generated_always($k))return
null;$nf=bracket_escape($k["field"]);$p=isset($_POST["function"][$nf])?$_POST["function"][$nf]:"";if($p=="orig")return(preg_match('~^CURRENT_TIMESTAMP~i',$k["on_update"])?idf_escape($k["field"]):false);if($p=="NULL")return
Driver::get()->getNull();if(is_blob($k)&&ini_bool("file_uploads")){$m=get_file("fields-$nf");if(!is_string($m))return
false;return
Driver::get()->quoteBinary($m);}$Y=isset($_POST["fields"][$nf])?$_POST["fields"][$nf]:(isset($_FILES["fields"]["name"][$nf])?$_FILES["fields"]["name"][$nf]:null);if($Y===null)return
false;if($k["auto_increment"]&&$Y=="")return
null;if($k["type"]=="set")$Y=implode(",",(array)$Y);if($p=="json"){$Y=json_decode($Y,true);if(!is_array($Y))return
false;return$Y;}if($p!="SQL"&&isset($k["full_type"]))Admin::get()->detectJson($k["full_type"],$Y,false);return
Admin::get()->processFieldInput($k,$Y,$p);}function
search_tables(){$_GET["where"][0]["val"]=$_POST["query"];$ek=$Ad=[];foreach(table_status("",true)as$Q=>$R){$Ml=Admin::get()->getTableName($R);if(!isset($R["Engine"])||$Ml==""||($_POST["tables"]&&!in_array($Q,$_POST["tables"])))continue;$I=Connection::get()->query("SELECT".limit("1 FROM ".table($Q)," WHERE ".implode(" AND ",Admin::get()->processSelectionSearch(fields($Q),[])),1));if($I&&!$I->fetchRow())continue;$x=h(ME."select=".urlencode($Q)."&where[0][op]=".urlencode($_GET["where"][0]["op"])."&where[0][val]=".urlencode($_GET["where"][0]["val"]));if($I)$ek[]="<li><a href='$x'>".icon("search")."$Ml</a></li>";else$Ad[]="<div class='error'><a href='$x'>$Ml</a>: ".error()."</div>";}if($ek)echo"<ul class='links'>\n",implode("\n",$ek),"</ul>\n";if($Ad)echo
implode("\n",$Ad),"\n";if(!$ek&&!$Ad)echo"<p class='message'>".lang(79)."</p>\n";}function
help_script($cm,$Xk=false){return
script("initHelpFor(qsl('select, input'), '".h($cm)."', $Xk);","");}function
help_script_command($Kb,$Xk=false){return
script("initHelpFor(qsl('select, input'), (value) => { return $Kb; }, $Xk);","");}function
edit_form($Q,$l,$K,$Qm){$Ml=Admin::get()->getTableName(table_status1($Q,true));$T=$Qm?lang(39):lang(114);page_header("$T: $Ml",["select"=>[$Q,$Ml],$T]);if($K===false){echo"<p class='error'>".lang(90)."\n";return;}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";$nd=false;$zn=($Qm&&!isset($_GET["select"])?where_columns($_GET,$l):[]);$ac=(count($zn)!=count($l));if(!$ac)$zn=[];if(!$l)echo"<p class='error'>".lang(115)."\n";else{echo"<table class='box'>".script("qsl('table').onkeydown = onEditingKeydown;");$Qa=!$_POST;foreach($l
as$A=>$k){echo"<tr".(isset($zn[$A])?" class='where-column'":"")."><th>".Admin::get()->getFieldName($k);$u=bracket_escape($A);$i=isset($_GET["preset"][$u])?$_GET["preset"][$u]:null;if($i===null){$i=$k["default"];if($k["type"]=="bit"&&preg_match("~^b'([01]*)'\$~",$i,$Wj))$i=$Wj[1];if(DIALECT=="sql"&&preg_match('~binary~',$k["type"]))$i=bin2hex($i);}$Y=($K!==null?($K[$A]!=""&&DIALECT=="sql"&&preg_match("~enum|set~",$k["type"])&&is_array($K[$A])?implode(",",$K[$A]):(is_bool($K[$A])?+$K[$A]:$K[$A])):(!$Qm&&$k["auto_increment"]?"":(isset($_GET["select"])?false:$i)));if(!$_POST["save"]&&is_string($Y))$Y=Admin::get()->formatFieldValue($Y,$k);if(($Qm&&!isset($k["privileges"]["update"]))||is_generated_always($k)){echo"<td class='function'></td><td>";if($Qm||!$k["generated"])echo
select_value($Y,'',$k,null);else
echo"<code class='jush-".DIALECT."'>",h($Y),"</code>";echo"</td>";}else{$nd=true;$p=($_POST["save"]?isset($_POST["function"][$u])?$_POST["function"][$u]:"":($Qm&&preg_match('~^CURRENT_TIMESTAMP~i',$k["on_update"])?"now":($Y===false?null:($Y!==null?'':'NULL'))));if(!$_POST&&!$Qm&&$Y==$k["default"]&&preg_match('~^[\w.]+\(~',$Y))$p="SQL";if(preg_match("~time~",$k["type"])&&preg_match('~^CURRENT_TIMESTAMP~i',$Y)){$Y="";$p="now";}if($k["type"]=="uuid"&&$Y=="uuid()"){$Y="";$p="uuid";}if($Qa!==false)$Qa=($k["auto_increment"]||$p=="now"||$p=="uuid"?null:true);input($k,$Y,$p,(bool)$Qa);if($Qa)$Qa=false;}echo"\n";}if(!support("table")&&!fields($Q))echo"<tr>"."<th><input class='input' name='field_keys[]'>".script("qsl('input').oninput = fieldChange;","")."<td class='function'>".html_select("field_funs[]",Admin::get()->getFieldFunctions(["null"=>isset($_GET["select"])]))."<td><input class='input' name='field_vals[]'>"."\n";echo"</table>\n",script("initToggles(gid('form'));");if($zn)echo
script("initWhereChange();");}echo"<p>";if($nd){echo"<input type='submit' class='button default' value='".lang(116)."'>\n";if(!isset($_GET["select"])&&$ac){$Pc=($zn&&Admin::get()->getErrors()?" disabled":"");echo"<input type='submit' class='button' name='insert' value='".($Qm?lang(117):lang(118))."' title='Ctrl+Shift+Enter'$Pc>\n",($Qm?script("qsl('input').onclick = function () { return !ajaxForm(this.form, '".js_escape(lang(119))."', this); };"):"");}}echo($Qm?"<input type='submit' class='button' name='delete' value='".lang(120)."'>".confirm()."\n":"");if(isset($_GET["select"]))print_hidden_fields(["check"=>(array)$_POST["check"],"clone"=>$_POST["clone"],"all"=>$_POST["all"]]);echo
input_hidden("referer",isset($_POST["referer"])?$_POST["referer"]:$_SERVER["HTTP_REFERER"]),input_hidden("save","1"),input_token(),"</form>\n";}function
file_upload_form_script($re,$Af){$Vg=ini_get("max_file_uploads");$bh=ini_get("upload_max_filesize");$ch=ini_bytes("upload_max_filesize");return
script("initFilesUploadForm('".js_escape($re)."', '".js_escape($Af)."', "."$Vg, '".js_escape(lang(121,$Vg,"'max_file_uploads'"))."', "."$ch, '".js_escape(lang(122,$bh,"'upload_max_filesize'"))."')");}function
compress_alphabet(){return
strtr(implode(range('"','~')),"'\\","!\n");}function
decompress_string($P,$Kc=""){$Fa=array_flip(str_split(compress_alphabet()));$v=strlen($P);$en=($v?13*($v-1)/2-$Fa[$P[0]]:0);$Ya="";$ck=0;$dk=0;for($q=1;$q<$v;$q+=2){$ck=($ck<<13)+$Fa[$P[$q]]*93+$Fa[$P[$q+1]];$dk+=13;while($dk>=8&&$en>=8){$dk-=8;$en-=8;$Ya
.=chr($ck>>$dk);$ck&=(1<<$dk)-1;}}if($Ya=="")return"";if($Kc!=""&&function_exists('inflate_init'))return
inflate_add(inflate_init(ZLIB_ENCODING_RAW,['dictionary'=>$Kc]),$Ya,ZLIB_FINISH);return$Kc==""&&function_exists('gzinflate')?gzinflate($Ya):inflate($Ya,$Kc);}function
inflate($Ya,$Kc=""){$zg=[3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258];$_g=[0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0];$Sc=[1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577];$Uc=[0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13];$J=$Kc;$G=0;do{$he=inflate_bits($Ya,$G,1);$U=inflate_bits($Ya,$G,2);if(!$U){$G=($G+7)&~7;$v=inflate_bits($Ya,$G,16);$G+=16;$J
.=substr($Ya,$G>>3,$v);$G+=$v<<3;}else{if($U==1){$Lg=array_merge(array_fill(0,144,8),array_fill(0,112,9),array_fill(0,24,7),array_fill(0,8,8));$Vc=array_fill(0,30,5);}else{$Kg=inflate_bits($Ya,$G,5)+257;$Tc=inflate_bits($Ya,$G,5)+1;$D=[16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15];$nh=array_fill(0,19,0);$mh=inflate_bits($Ya,$G,4)+4;for($q=0;$q<$mh;$q++)$nh[$D[$q]]=inflate_bits($Ya,$G,3);$oh=inflate_table($nh);$Ag=[];while(count($Ag)<$Kg+$Tc){$Cl=inflate_symbol($Ya,$G,$oh);if($Cl==16)$Ag=array_merge($Ag,array_fill(0,inflate_bits($Ya,$G,2)+3,end($Ag)));elseif($Cl==17)$Ag=array_merge($Ag,array_fill(0,inflate_bits($Ya,$G,3)+3,0));elseif($Cl==18)$Ag=array_merge($Ag,array_fill(0,inflate_bits($Ya,$G,7)+11,0));else$Ag[]=$Cl;}$Lg=array_slice($Ag,0,$Kg);$Vc=array_slice($Ag,$Kg);}$Mg=inflate_table($Lg);$Xc=inflate_table($Vc);while(($Cl=inflate_symbol($Ya,$G,$Mg))!=256){if($Cl<256)$J
.=chr($Cl);else{$v=$zg[$Cl-257]+inflate_bits($Ya,$G,$_g[$Cl-257]);$Wc=inflate_symbol($Ya,$G,$Xc);$Xh=strlen($J)-$Sc[$Wc]-inflate_bits($Ya,$G,$Uc[$Wc]);for($q=0;$q<$v;$q++)$J
.=$J[$Xh+$q];}}}}while(!$he);return$Kc==""?$J:substr($J,strlen($Kc));}function
inflate_bits($Ya,&$G,$gc){$J=0;for($q=0;$q<$gc;$q++){$J+=((ord($Ya[$G>>3])>>($G&7))&1)<<$q;$G++;}return$J;}function
inflate_table(array$Ag){$Q=[];$_b=0;for($Za=1;$Za<=max($Ag);$Za++){foreach($Ag
as$Cl=>$v){if($v==$Za){$Q[$Za][$_b]=$Cl;$_b++;}}$_b<<=1;}return$Q;}function
inflate_symbol($Ya,&$G,array$Q){$_b=0;$Za=0;do{$_b=($_b<<1)+inflate_bits($Ya,$G,1);$Za++;}while(!isset($Q[$Za][$_b]));return$Q[$Za][$_b];}if(isset($_GET["file"]))load_compiled_file($_GET["file"]);function
load_compiled_file($n){if($n==""){http_response_code(404);exit;}if($_SERVER["HTTP_IF_MODIFIED_SINCE"]){http_response_code(304);exit;}header("Expires: ".gmdate("D, d M Y H:i:s",time()+365*24*60*60)." GMT");header("Last-Modified: ".gmdate("D, d M Y H:i:s")." GMT");header("Cache-Control: immutable");ini_set("zlib.output_compression","1");$Od=pathinfo($n,PATHINFO_EXTENSION);switch($Od){case"css":header("Content-Type: text/css; charset=utf-8");break;case"js":header("Content-Type: text/javascript; charset=utf-8");break;case"ico":header("Content-Type: image/x-icon");break;case"png":header("Content-Type: image/png");break;case"svg":header("Content-Type: image/svg+xml");break;}switch($n){case'favicon-blue-0f5ce53a66b1e25395d0048da369f19e__519803fd.ico':$f='AAABAAEAICAAAAEAIAC6AQAAFgAAAIlQTkcNChoKAAAADUlIRFIAAAAgAAAAIAgGAAAAc3p69AAAAYFJREFUeNrV1wEEGmEYh/FztCYBRATANhCAAEGAEGZowEUFhM2G6A4QAJksoMi2AYRlAxgcAUgthAS2yTFo5d2DDzbO6r2PhB9APY73z+cUn3+6qbsJcFGCjxlCbPHL2CLEDD5KcG0EPESAH5ArfUeAtDbgCb5BElrjsSbgI8SSD5qAM8SSsyZAbNIErCGWrDQBTYglTe0ZNnCAKB3gJR2iAnwsIBdawEchyRC9jompoYUe3hg9tFCL+dNX2ivo4wEcpTT6EF0AsEMHeTgXyqODnf4M489phC7aeGq00cUIK1s7sLr1DryEWPJCE5DBBJLQBJkkO9DAHnKlPbwkO/AMjuGijCGWiCD/iLDEEGW4f/2WIuA3qnBiZPHIyMKJUcVJe4ZHDJCDc6UcBjhqz/AEMSKMUf9PTA51jBFBAN0X+AKJEWGDr8YGESTGZ02AB7HE0wSk8B6S0DuktDvgYgRRegvXxsuogjnkQnNUrL8NUUSAKUL8NEJMEaB4x4/TG/gDMBOIUjRp9w0AAAAASUVORK5CYII=';break;case'favicon-blue-17e440832c1eac07527560a0d6f0d2ee__519803fd.svg':$f='+<bATb3V?$so%el,wEIwK&mlYjGZ$a8-HGs8y$j)-UBmQx`Cf?>]C6?xmhS1<w
ZNSJl63"aZ]nB<rgk|tG)vC,pHYx;Wb2NVXBMxd!lQ=g0"!mM[]c*SW?7
E^]B7t[fHolNczfsymCW
CM~8Ult[(brlOx`71nDc
H;N~ybgNd*l6LMbRk;>%06rQeBnDc14r_7Z0b9uqO=xV5=22d05hr#V]F`V>ZD,#JA9[$XEV-*TBfz7Z%
rEJw!G!cT+[7>-u:iVU)N$iv%ySN.`._&}sV@FUIuH)%cY-0#OXWPT8t./3Oid8~R]3?9n;/Qs
Y2!K`1Q9d
tys6C=xmJfXFCT%0xl`H&%njK7`N[.q6jET6kV
VqmiJoIrZ#rsP~q0vDN<FH1w9l4R-?A#H:#onn0@0]3dNk3,E{<7r;2q
u)F!d(nhskXC~JT4N!~!g52r#`3hF[%j
oPE~ZV(W_~g#t?WXQCxEe7)ZQKGxei4.gu_R>pAL]HDZf!uSL)$_)^vZ:Xk![_HKh=C|S~PjqHcgjUutq#-~?PmA#<MYyg2R';break;case'apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__519803fd.png':$f='iVBORw0KGgoAAAANSUhEUgAAALQAAAC0CAIAAACyr5FlAAAK7UlEQVR42uzSgQAAAAACoP2ln2CDYig9QA7kQA7kQA7kQA7kQA7kQA6QAzmQAzmQAzmQAzmQAzlADuRADuRADuRADuRADuQAOZADOZADOcbeOUa502RxeG0bX17btm3b/tu2bdu2PbZt21bY+6xqPduTSSddSd1zPyTdVZk+p5+prr51f7d83ixVOXXH55Yte6No2r25Qy/O/Pg7OB/4ykFO0cC/4FBma6qq2Tsmb+SVGe9/7f86zWhMFx+HQ5mjs7XmwITMT77LXe+R04WOdFdw+Ka1xOzL7vML7rTLTnd+RMHhW+Z01Owfz911izOE8IMKDl8wh6W9dPHLbsFCOD/Izyo4pB8z3EyG4GPJK5rTqeCQ2HgEGECGeL5MVHBIPAM1CAvh/AkFh5Rvrf/13STr0x/UHpmO4yXzn+7+3pev+VA0puN/fX/hDyk4fOSBkjPg96KN09KRN/KqbuBoSzsnGtNRz8NFwSFBDJRAeDdwCOsqS8/86Nu9gYP4mK25WsGBaXVNlpS8plORFeuP5k/ZkPbVnLj3J0e9Nib8uWEhj/QPvPvz8zd/eAa/54vzfOUgp2hAs6kb0uhCR7rXN1s0I42AN7dNDxxYY/CG3sCB1+wb66dwNLVaQ5Jqlu/L/XxWLPf+hvdOu8X5qS9mxS7fnxuaVNvcZtXcaiyI6IeDN9KyFW/3Bg6eTf4FR1p+0/ydWS+ODL3xfe6lsc6feGlk6IKdWWkFzW5Za+WG6YNDzF5bWIx1GQ7cUpXr+3DklLQs3pP9zNBg7plX/NmhIUv25OSWuv4KwFK7TjhsjZXic2dRQsYH3/h3OFLP6oSDP+rLcDAV6DM3jttjEu83P961gYRUDJ1wVGz8wtZYIb7Wn136b41bk0/phKNs+Zu+CUd6QbPAwmzeb158ZlHPECFVRy8cGz6nsdNuE5OP0kUv/gscCUd0wlE07T5fg4PFgQ3HCni5MCEWwrm8zScKNd3G7EEnHDX7xnGkes9occTe3pgz8I//HgDVAUfu0Et8Cg5eQxi6xT0wuQ9amNDSbtN0GBEOnXDUn1nytxEi6aQ42JEbIRo3hW/XCQfRDt+Bw2JzvDk+QgoshL89MdJqc7gRjqawrRzBs774ibWuWByvOzabg3hj4Fp/hGPaxnSJsBA+a2uGGx8rLXGHxPHCSXc47Vax0F8853EO1p2c73ePFeJO0mEhPDKtzl0T0raUM+I4XrVjiDhlb67J7vdrQp9+NyGdKuGwIXzWlgx3vcoS4/q3s4wl4mx7RkDlxi/97lWWuKe8cLw6JtzFIJgOOLI++6G1pkA06CyI9bsgmKRYiDdbF8PnOuDAC8bf7LRZRBuZwucKjps+OKNn4c1lOPDKLf1EG19beFNwsGTfGzjw5ug9oplPLdkrOEj2IfDQGzgyP/2+EMr6UrKPgkOkCboOB54/5jqntVP+NEEFh0sJxuQPc6QbL5n/jN8lGN/+8Vl54bjrs3NKmmCgfTg1Wl44Pp0Zo0RNBtri3dnywkE2q5JDGmikz4gcDrn81o/OksjodSE1ZPiykHrt4XwZ4dh4vMA1OTXPF3c+TcSY4ZNwOJzOT6bHyEUG2ginUxVv8Yi1d9oHL06UhYzhS5M6uuzuKPs00dWyTxP9q+wT/4hrDuUhGzF1YOP9M0jl3CuWJODdvUpWOM1oTAzUTwvGRafXvzHOpCmDb02IiM2oN6zUZC5L7aRikKpDKhfDA84HvnKQUzTQVKlJhpDTkZXPDQ8xDxYvjAg9G13ldKoiteYwu915LKyceR8juRcfImhoj4dXOBym5EKVmqxu6Nx0vACBvCexeH1c+JYThTUNXZoyKVT25bUdjCWUWkDobIh+elQYpRmOh5VX1HZoyswAxwdTo4orexzga2yxBCXU8M89fVP6l3Niqb1xS0/CrDSmC4U66M6PBCfUNLZatR5aQXnbR9OiFRzGLtkz9+x9VQymBRV1ncSzk3MbUQwExFefCK/YF1C6P6CUD3yNSqvjFA1o1vs5BHQ+PSRYLNkbZQoO/LOZMUJAZn5Dovfx9GiRz2GsKTjwdydFVtV3aqY3Bp63J0RwwQoOj2aCPdQ3ICK1TjOxhSXXPtAngEtVcHgnTZBFuIiUOhNiwfRTXKSCwxPWTaz6XEyVw9tRSS7gTFSliOgrOEyUYIxY8nBImc3uBUT4oweDSp8fLmIqCg5TZp8/3C9gxPKk3eeKqd1m6FDCj/Ouu+tc8fBlSUyAxAUoOOSQJjzw1QUK62w9WUgFN8IVblnESc1ropjTwIUJ9391wQBpgjdM6Vbu/uw8AZIxK5PnbMskF4ShheVcgl1ZRS2VdZ2dlr/l4/CBrySrEhyjAc1oPHtrJh3pjrzAAN2K6U2Jmm7/5JyhuhgFh7HG/ZNXmsDQpeAw0N6bHCUvHGrhzVibuTlDXjjmbs9UcBho7GshLxyE6RQcxoYgqekpIxls3UJcRMFhuCKSab901cCEFlLBYawt25cjFxwrD+SqNEHPGYp1WchYdTBP5ZB62rafLjK/4o0Aq+YVU9nnF+KqHxsQZE4yHh8YRFazhik4vCiqZkc3UxXtIFt90a5sIZ5WcHjZ8spa2bLJ608ZLqDvvLj8slbNbKZETUha5m3PvO/LC57HguV7BrDiqnZNmbfg0JNR3GWxkw+Gbva2jwwvPcgq7pezY4+ElHdZHfovXsFhiDH/Z2s3nY2hhNxj/qHdK53l2fH62PCFu7K52TChv1oVsCo4DLS/ZkUkZjdqPbSGFktsZv2+CyWsfjEtQH+mU49PM/aqpQsPLCRx/AjyNa2HRi8uG9eMNpXs88SgoNqmrt6nBCN5RW9NYDspt5ExhoUxnA985SCnaND7dGXU948OCPRcso/KBHtqcHBhRZtmessvb3tyUJDKBPOE/dsLgni+mNPisxrEq5OCw3D7z9VOahoz8dRMZmQpM2Pl8rycQ6oSjJkwMlEwlRaSp55KMPa0dZ+hyVqG0+nN0nUU9vhgSpTKPjepNIEiT7xwincZz1htY9feCyUUEVTSBDl0K+zTSfhLxKncbkx0wlNq5+3IemV0mCl0KwoOBGcuSJWoA0YtL7ck6mWXtKCFpKSkC/In3lw005rSrTzYJ+DN8RGUfhu7KoVxhZ0MWIUJSapJy2+iKCBjDOMBH/gaklhzKLiMBjSjMV0orEB38+pWFByUjpRXmjBjc4aCw0Cj3oG8cDBTVnAYaCx6oTiVkYx7vjjf1GpVcBhrpPnLCAdL9irZx3Br67SJEn2yOMWGPJRSqtIEqb4lFxykiqk0Qc8Zy1qykLF0b44mv0m2jdeoFcnmJ2Pc6hTN86ayzyl/TnKvmclALUEimYLDaxs0kdppTjJ48Nm9TobSrZD4effn500V0kCnqQlTcHhdzmSSbSLZI6Gk2jTSJgWHKPpzNLT8SZGC5XFH7sDufyLbSMFhxp1vtp4qonCxJ7EgKEc9CDn2B1IbALa220jgQJdmNBY8y2CRoK0miyk4hFEbn2oIbq/hgapq8Z7sPKnV9AoOMR1BkMjSF9XsH+kX6BoQj/QPpMb+uiP5qFF0TCwUHHIade/ZTnzJnpyJ61IHL0pgdz7yQFEskvlHTiEf+MpBTtGA4DevylLsJ/endulABgAAAGCQv/U9vmJIDuRADuRADpADOZADOZADOZADOZDjBTmQAzmQAzmQAzmQAzmQA+RADuRADuRADuRADuRADpADOZADOZADOZADOZADOSAe34f5izVe/wAAAABJRU5ErkJggg==';break;case'logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg':$f='(]^+JbP.FqjXYdorFxH%oTmn1#,Na[(-^<}T{`+Ahl-RItQoM;{4bK}l["$V3F6U&V6Ey@S8#w=t>3kaN[hLow+fWEUH+K<LoXqyEy6JupFy-JyK4S8q(7tl96;KLl/F|,Cz)p?p(B)[axu/4u77-)nvU
R?vPex0x2ynqlE!VMsqy.7^Mtiv[hKzB^oh,VovqjM1XCS0v]mXW-smT}3TK7IVEL2YtHsc^Dne,}uyaN:]l/HJnieEbYSTw;KD$c_8p_B2y&,]pd?W+OvtUWi,FjFuW3Gsr=[=,k5ZhU;]w50sP*<)SM
tcO5=+WoZrY8Iq)IW=_gPo=RG*5hngIJV?j"daOWXS`x~L$e])]A/t{9it,:r%.89Z!;1rZhBw]6K6fQlvHN$Hw,QuiFcFpKmc{y#sO=!8QV,<+O&P/25]6vLiFL^ILo%v=7LZHx2=IpuT_qcxR7puVAY]-[aZk-!Hsk3@pU2?.=/khk7TY+8^U^mMe^&3|d[5+h9;Y
kr~/LPx3%=u>(#a3Hf@EX)<u
hpxoYBBVp`W(PvmMW
B#sK.gGL@Vd{:",35}yAFD8*Arm#eht>.nM#/VX$c0nfYn>@aFR7y~^p#M;>Hr]/"5-YOhURoN?g"zr)rf03v&=U+I-CNf2fyI`@2rCNwy$T>{3b.C"<mw^pUpNV.:1gW1HboUDhY6rSWb#t&3^ZZCWe([&88L?Tb:rJC{:,[0cUZh4Z?E>_4(eVbK+W4cj3K
6JZ,1OCPNi-r:-0+h9c@$6(OPFO,>/K_<D>?aD4|c[qNng
#]abQba^dg.vgT
jO4.nVHH3Y??RBOkYeEql7Z%i$fv:!`8=ol
<6HDyKdV^.GOQE<w848Z0)$;-[WOZ($QN.)/E#@[UhS3g@bs8$w@iRav#q,^!">riV0ad4mzAx-tm;I$7+G<hFV$knOjWB`9D:,!6.B`@~D~lLM@<M0y2w8SF<2z*Q?8suZ!O(%O"i>PX9(r?[=%/{TBK"Y5o,?wUbppvc%SDB9:2sH.!E?uV/?
m,@iTyWH"kU~.Qf,)]TyKwNyoX6LeQ(^HfM@6j
4o+qU-cQZ:uU]TVg=la`BE{x<YgRQys@]DNHkxs-[I/xZDH(tx~I,OKPNZ/@fA]-^.jOn630BkZbx.P^-,m);cooD1IAp.,``B4+,etGxX"U8fa;-m84^sKe*v>@/HAeYMWEKTQ)eqhf~:)bj!p<2bBA{<+-LC46:QPR:9CjzQATX#[YXUysw]
N.c{F{GlQ+bj=,TT-!C{[nb4XXv@IXBg4/"YW.M7"&I]1:iT"%EKDl:j![3j6cJm@H6qxXW2/Z3Cbs2d^_Mps>DM!ccnZ<i*Bk_oLtHcB*IHFOrym<(YWVBvJs)l@)0Z
=r:E0<}*va7n8dz1"9z&IAIi9Vql_/_GmWkv_:7+J@p:0<f]@QLtEi=rp`*wKM:5vfI1|nK.ne&[~?Dw9$GKV(o;/%`Hmip$>""Ue?0@$iQ%0E@-8u^"L:b>FLzv@>2F,<8Oa+M=?1oWnKWe[PvjmLPP1h}>?=m6-g]sv1UozX%`5v(*-1kTxb9=scVhWiuXQq$+!BPCVI)xDF&Cnc4ACZZ;UYX0(]s_GY!vk8WEz/4F"DLf=_6%>e[r;9[xM
*??SKd):Aiccqb{<(e68*v9Xya1
}IiKS_We9OJP11tEgIuGCfq=227bEC06#b8:]191/`0PF4dN3NCRTej;PMj)t1HQ
Jk-U9uH!E]5fjhHQ[+SE@:i^g{tA^Al~K<U3Js9&fM#B=^50#vEFbxFZ5L?Y3#pI^GKK[GYdMVSZS-kM<^><@^4f#(*V&b>jq3*^KjD3*Rj:sZUT"F5[bbKNE?X;A{TeBBBDDh+O^.lXKwEfA")l6+[^TWA?4gsuw|<
F:E?URQb2aF,p$7S90=|txQTehv2K|GQ]/#8t!]{/N<29Gp"TPCb9HnMc}q@$*7z?v`WcA(@>t%Q%t2zFCg.^la~3eLCq_$QqZ>erybXCLsr`Q)1Xvng-<,eXT8Gismd[Kh5k)PClZRUu<<uVag@F,=B#6wrEv$LS(Zs^CXo2:d/o%A%n/ZC1%4vJi9[DJ7|ViE>Q+(A:M5wRMExVg">y+d/OirXu6Z@>[`*:xk1k:,a64QavY*$xgkb=eYrj?%BUFsiBT>VTy`OsXZ8T]!(4(9)TVb`f![p</Q,?n.6x;Vcy,ezD|@X0Xca@ad["tI%:wj?P}^e*sm]oP?U`&OhkEg+TWAAc5FL5H.DImYqS
4fIvQ%7XhX2^!kthV*<ddA1ed`@9m6@mZ7)ocp_a%uQl@q0U??@Nf?_0.+DqepA/LGctQ1X(#=m3EmLVkn?I+7r~foFQN=BUF~8$nF"4
{9LE>C+$c%w8vSvNB?}93S#K4kkm/+t;`RE%e
;(Yq`=YE$3,5|@/mXG|%z7YbGWWmFH+IC;f:U$J6vpyr)hV7nU62e3PYwL~yj.31;lgq)EK$.>bGaY5`n6mlwn3@/u`vQ:9lc4J6.&&ga%^j!0joCA$LU&#g7trww.(CN=l2:G1CfPS:KH=d>Hfpi7[$X`,7wJZoJvRF?YKmSVzNW0`Q0FdOqAIS
n9lrNOU01c?:p/5y16+Zkgo}`M)D6xm>7RiQ_b%p%ocllip!0).myrT"w[53iGRZBt8z<.d6p9("_WYy1v;v.xBx%3c3&hfawJgMtuxeflyK1.-:wQo"f_z&)W';break;case'jush-b3a93b18444da26820ff61746521dede__21b065eb.css':$f='%X.mPb3V?!K0u25Dm[994[Zg@N#Q)YOC=2R_hE~4)=>cbdia55M!*B>tthOR~qe;I4(JE7nkwCcvDp&`dDJK
f-B>jO*,sE/El7[<CeY-WtHaF_.kmd7o@TK)f,bmf|yor7&?P^<qeg8="Q1IULQ7R}m#!4#2";]EMU!F1OpTIT56:qJms/Dv-ge06DN|n`FW&*;(q:eY(cbjkU?x]gUap[%36X.0y?`E_>T!nUPeLxWQNBQNxgW
`cW13j?"h~G+;>ckmN2+
!n%Sl8/pL[B8
M3BQ=4I8t<Wims^cdx*+69AM:3(4#D7NrL5JU;F-nR)NxQWs7{b*3#%wvQ^i5"h{.StgdiF@(/BD"LK.,r2{$e&b4#$3x
`jO"8tQQ1CKB,TYk$:TIGnJ]L
WcYUpj,c5;pfH7%lebNsHuwv44..jP:
oZb0YJ@}qOxs&)5?s0oVA*
swu
jafJPvJQCQ5?:;JN^klF$8EV7&
J?y"=oJ8]]rh5ZK31++0s/h[@)W/]t4{Do0LZsyY]Ui=F:=Lj0OiN0Q{mj^W2%j7&7^of",u?pagL&"iw=-;EKZzP?gdKNd/SR.wcU#s5@q~Y}(R:VF$Q8dZ,L)ey.Hq2O=)g/K}w)n3hY`Mh>::q<VI?VE1nU^-^D/qa9JwIPcYCppQ/X@!6ZZjyFhzsSz&E^ke1)2F[wC7uibhCB4fEU_/mdWy=C$?"]4Jab*)fpfaP{Xx=d3$rPj<p5Yo$$mqn$a[gdSRbtw^Cd:8IL9Ep@:NwGhGJP+=>bUa%5!)#P![lKsPx,lj]_TA`s)}ui[?@K5<"=yhHX9ac1.(pCt<=mY#,*@Ho8y`eb"03`0_6Wx1`zEp[UTJnLo&k`Z*%U;u_AP@.ah.ussF)SKcuGHm@)HJg~D|K@x[(QcKT7b133MD8{9lxGHdF&gk".u/dfBj;$`frs[]W0RtuZ<]q+TW7f)<cBALJpw,4G-PO&to>Pl>;:<vNY$aUe;BxF$7V3N~j=h.gDd*D(06YWAm/nioW)g>F(ql[`!L/h`[Z(V`2J';break;case'jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css':$f=',Gjwm6?!R"-YJmoGR`r@~cEv;#i.*-_KUyr[0$CF,>/n=#+liP*01.(73:+G.C]Ek+^-h&|hnGDq1:ccpxU98SxFh5MU%c+]DCcezAcUOWmDiL$
)yZA,ICx<`.i#E%U;lo*kf6u&LQx+!%1t]iP#G9;zGT,4U2"ha>hB#am`y1YU6$z!l#C%';break;case'jush-615bc0b9720a1de8edd2c6876a3495b6__12a85d92.js':$f=',hk]`!>p9CvwpHP(hqgS0$0>DjvvnfNtZ=+9nN
$.nl"6@fn<kV(<^:]t^9c|n2m~.TGXrpInd(UuxUWLkl
*BAM/ne)r4ogC<0Eg7_(|eIi=nSi_k{<p@N9~i>R.x"MZP=Dy74;<h~shEg7y3hwym~^?k{HAqiq>Yqi$s>S_nae`<bE.Kljt`?7de~t|wBqj]sHR98]oS9mWC,xq"pC4jtZ5NRB[[Emsy&Xo_5yDw=v}1=j$oH^H#<R,cTl6y.O&I.lx8Z)zP"m:mIB}4bNu2UC;NO*GRku0y.XaxKOxdZhs!/HGK]/%.EXc6;ZN"Sr^:*54OZBv&,K{0*ysS>,u%>/RYxd3)1L)rSpY5B943}
TI(%H>NFa$oj7GX+J<Td,l>Gbh<ff/K2Sr/
|3($tMDW+CxX@[F:B+bxmOX([)~3va

i2SByA*^g/8uHsueh=y<#36+byU&)($&l4;IZCY4v+{v$Syqu?KrH&>51fyfW`>/Rv9U"y4mS<3R@cHL+l!oNgeXKa%qgTtK9@v&_2Sd%:;6-HNfeqmDHLV[Y4i
A!#jUa,L-dA:H##HDk$e{U[xAqdZ}oDEhI=LR2PpjJ=31^W&j!K+-+9T$j1BN^"y62hO8!~58w/]=B[I}<.A@!!AyD}IO0A"dE-8"2_&hIJuz,mRK0wQ-5&<]6^.@HG@
0<@|nj=oK[t-Pz44a-KS&{ktf#"Wa>bN"Z0w6/GZ*}2]?%T>Tn)oS<8Djxbe?*5!&|I5c<nhb<y^[3.b[*`>-Br*mk:-Z:WmiMVDe?0D6i(@fMKQC):E?6
u4@T2w$YJ?NB7f-YRSDd^,=uWTQH"TmXRIF4&F5ow"}Tx?&G]w^yB:UD6T"mv
[iNJG9#
F_`]&Qnkt!C^Oy!`EKsW0TsmX@*l-IBhjBO[jaO,syHtMP+M[*LV1Zj<x%-]XyR2gof;Q8KwoX2m=p!mQiJBW`VMan-D]h:t(u:;Q?@1UYj!^&>isfU0.GMX:$wY9/AL?9=G7s;7RuiWNk[)"k_kA)TB{v&947loVGF[Upw//1``bNPODu,_FR!LIJgX^v"o]nhewizy3j1Ht
d$K$RLeU1xDM?[{IT6z,BUZ6Gid6vo"SVV=$VeV;S+5jI+XGP^KvUCE$Za$fIfMSVR[c_FoZ:x4.~kWawhx9:$O7TB?P=fA&>RhI*=.n6V8xsTMGN]!#qN7Xy%b9hi!X!nmqgGVXUxnTiX`A*O+)D>bkxXBi=yq`?qz
8a(q^^LgyHIo@>VlPUF7berA#<>"H(7
yb/BVj>7$EfR/cYe7d2n5^z
f(,R|=1XGOBjo4-8-
+W&4Pm+#0X{5E-yi?q%kNEv58#*Ty6D+@qKE],&l$Vi9=aJ?CRsue;p7ohg
v#=UFvY%QWf0O;3c.`K8a4qJlxNDk==XYt^T`GRY!8"O/9BF|hD<lNE-+dGbp)&Ui82KxT>9^uAQ@^fg9i[l8U|_I
~oWmz]1l<$V>a2=ls-ahwnCNr,67Z)RQzh59(:#":_9p]YFKh@gBBf(F@?PBQ-Z?lL5%8DO^D*Ysmj%exK5(Fd-DJ(5UH3647v|d,gh`En9Aq/@7qYYn3ZMI*pN)]H[_9)Q:!10=Da?&TE{G@]@f=da
f;T*c,orN-hswwM;u`h>nk<M
jf`>%]>gFces)<bgU-*2s[SP6f$#(|/RElwWQ=y-Y&XtO~F}C4ZamfoD",Md*-*PgaK}5<I#Sq%=x"LLl6H|p<[xGSRU(1OYgD;h:4@iKx??A9qrgGt<d.qL6i)iFG+pt,&ZD*j
]b@uC{b*pc]UD~/)yUq{sJ%!1RwA=1fvhM=*fzwD-~$/C.ROPioy@R)w1>%@Q[kHANX=%|s3x<cTXP5vLp_uKfuBFj_;?xa<LxKd+r]^IM%hE7BoQxpl+W[Iv=j|c/<m571g7N!rVX)>j!e+fuK]6"q`f^jJ2YpA5{f4FeiVL
ui=^3lnpazny(dIHWXON4rsY_?xTW,"Wo
8D*zj{bwr7TQ)d
v
F<#"ew%h>35k/2$+j8Fe=aCvga%sEbRx5Ag/g4d7?<;Og:YPAq_)+@=GJR4Sz6g0"q%rTEZC)D`C9*lF{9lF[bAT3llO_L17Q[BSMN.;E=c7cT$&(QEw.Wl!YM7fFfvhjmZ0_*wa1pp)w2;^)o@4CJT/UdsKS:nn)1-=@#:1Fxb)NQ~oDACtd9.p55yW-2fS+C.kOQ>s#@Folk7/D06q>!5RjTLz%32hQ!.qZ#s+2T9/*!E$XVLUg^^uYUkev+F5h,fQXO.bQSi!(d70<:=-(>Q!>[(&`^4k_iz-qFD28Y2dnXc1Bp^GfHATo!7+21=UM/WATuKA#mt?tha3c(SUqT|9^[d(K@IYllExRxk=?(i/#cld<F=Dr>Y
4&"Yc_)U_Dp2|qcMJjfd_8bfoQM7|b[73K_?([/%3+D;W[tRvYJ-NZ[-%%(f14wZxN"[#u{*k;`.W#&nBDhq5g`)t/*Ne%WGm8a=.*ygQ9OB,$K89Wz)3^Kp+UMo=ETb}m,x"n.nZ1:m`#x,}FBm.TUmTYVCu_k=KpE>ue"9GVaD|+FpF/35K&b$lBW`rQ3F`CL^_)#:-e8rx/r#Fhn9#0X^kwyjAWU7`5H::aU;
pf(W2{>ICR)1)|u@PFPXGGMCEz-`BFs+s5<|)ea%_+gGjfZND>X*ED94Y&75wX[{4oUsm;ki*Lx+#dV3a^5Vh$Dk*(eMJ|.G_:QTj&k3dAa02<
qPP)1%CRn)0p6]abLPt!!YQc&QdLV?ds]g}?W]HMkg~;OXo3CnFiXUE+>bRPME;Vn[M!GAmOy"+[`!}pfPNpIIZQ?r=w{GYuYp^3BfH1Q`6j_NldO>N)Eke4SM4Z-I$APXH,-<FE0%SsA
*-<?8.eA7e;kA#Q@]7JBGs=f}0Ge*p811O6lltwh<uH,VRgO;&dvvu)i8T5=L*%EkXJgj?_c1[M-xXM=w[VVqr.K_Kof^OomE;05fCO"fN8?J?mN3y.4!ZScBkf5UR&%0A"EYp4nFZ|*IJ^ZM%+ej<H/yd8d`o&h[(7GX-uB#w`#_qsVj+s@?(E^_F
jw"}(S=B82iziq%AFc8`d_)o_XG|AK9(4?vIWK>N6<pK#y?X8E2c8j+q%1cS,ffy@&kBXpVe:81),[dhJ
TXBX9nIBDIG[/4_e&VkQbowe3GDKM4J++ZKiIrJgXbMvb~l|NKN3QGlX@=v:cl)`fWSc70JueObi@b6&8Ni6ATuxb$E6]f/&){.v6oO%$#SGD~sl_]r30`.d+x:d06C%ftM.0j`eYir<D&9}oIaqrfZ/-eT,im5)tA78gXLoUDH*]uDca(?4!U8yU$f4Xe;8^x<n35kEf#?bkWaC#d"?v[O.]lu}gui)adNkn^6Tf5<;F4H;sn#pl*#`-KbcUhC9lo>8ejm9W8EtcyZI]HQlW!qA`%5f&y]%&2-BG&A)AH8fx^JgO_A`oz8-$-cJ!N5Zh3sgM#:+b>si^$FqDZeCXSccstv16"m-NL@L2OZ@e*9R4jI$Z(*v:nI._|,,BNjn`aD.dBeO,~`f/m%e[CjBX38b3cwjYs*C@v2(A#FKS>`|f;=?@&A)2_6-.KVyZ"BK+FIDd[@|B#3D8[U]^zv_:41P"@Ie+S8QRlgQ3xwEMGHfoJbDyn*`T$3$4Eh?$1>p4EUzX3cMaj,S1qL;NOU5r%8)hSdH94#{1_dZ:rfKb5Y)%U;1(Dyf_U>(V?jZJr+J5s2rPl[#THqIxJ-ZW9f"4rK4sWr
j9xfcV*KVjwCx7:oeMwN@^3[MiTBI9Kc>!VcZ7mHN|hDD*;hoMl35ona%{ll,EuX"=SCMzx2XzT!yN*&)ty}SFq{bUaw[*bB5LJ.k~_`1^ynxE))Wu8v92&U.OuB-K3JNiCo$F;:]rct@E_dq[i>f5(O#=kd?Y8AWd0.fq$]"qZq3LDAn[CWyb!DF|j,_+`.JN*xY1C|FKjH6!M/.p01Pb))FWAR!IK#)[`_w`Qw)?ayw=xY%qK#&}++qm`~.W%>pC5p]ZysVicCw#LZvwTeG>[i.qG:,f?_]R4K09a8$b:COqE=I7hTXFH9?GT<`v4u^pnmV+"wx#r<R^ZG?-]IqJcpOKp)[zr]Z(^}9uDxegfCGs7nT<U`jYipsZFk;8*e.m?D/
D1hf^FhZabv}5Rg]&f`Sa1HNrGZ1.?6*-14^s0qRM7(q1JBPe7i+Q=cdHJq<`iE>3!5/0$75FR8:eEj{ySx>rh1Im&H!MYyO+(cyW,dgQds2z$_loxkL]2ae9w5-=f2URMm.g_g4,G`)L89BV,=8V[=oT8e2J!]lH7m8;1Sch]#PuDmRn/mDZ?&_B8Cw&)D=/J>]pph&(Rc;B^9TvN@-O]cMU?%.Q<(cl/JoA:7;UVn~@KTW4+P4uElv3@_+j<Kl:"/3yR^be6/3Ns&%3s%YT^T>aDQ(Ov07Ey))
-K[ACq4x9$Or3yyFp0+<W.vOux8a#cRXSc8D[.vp_s!;{LMjx%Q@W+JF%Ws5us+;s_z;$+xO94Nizw$"YnEahw#5mPW_*VB)a-ySl)cy9c8t0BmJugt_"G6?h*$bdl7Ba;(>YFCi3Fg;3HW+j
aroi3l[8<[!>l
pEka)m5etS)s$+Xsnz"^<saF+Us&dt";h`+4HALaxy&X+&fH*^pO8j-[E)KB)<!<njkA5ZPZ{%Q?6RpEphqayJYl}<4qhxy6WhFq^RjhV;xGy1dYNayYz7kbwt|?vi/f%66U
M9h$)dW5dU<=i0lY
{aNA`%!_O:i2"Ud>0_4a:>Y,5<-+YXkUtGq#R,j8OpwR},^P#HAaB&:Z:tPwoiOAs=^e.q|K-8DhK]_]}dc:XgeE=p]iI*5Jr3$Spix:FvpV2xQkjr`U8T)h^K,@H!C^Y
f;onNU~sBZ5gH-zuQ#WcchZKp/x;UE6^-glcKJlT8@l6|hu
h3&WEQHyzE)?O,CUeo;4Q#9OlVqQ?<]E}"x`N?fHKLu;CXF`vN/>Pk]kFT!sIEr`Z34d!BIj-+xYbcO8wG|?te5oO@=)N`gBCh$/kBiEmF,Nmy^"Zi`^}tw6iqhi5J7`+h59"<~-vB(ao0UFwEY@/*!G~g$.vy~aomokiJf-<lhObmb^}^Td,*_"Qlqqpq.B1MD60&eE"!k<QFB.xslP<
<#I5g.QVJJor|q:m"29Rc1C@*e!Tb21b$bFA&.>gf(A<YI!;aBIiYp]UQ5z/D`ZC6w<N^SM/zEGej6@hcJ^/fO5tabHF;R3!tkj:u/,Y&n%-;XA*M+xVIyb:L,":E^|1NSlL;^R3LGA1.BrTPN,yG5;0P(D6`N79E"Ypfb?2;kkM(A@x4qTYy/QqNMPCC-?V-kHGMyE;gN#Juyu`;p;9rhX+c=@??V>[7.5Up#@r+g^nh<-MZ6?9,d4dXxUy-)8c~mP2PZk*~WiT$aiN=wa<|HwT4JT7yR!4)@>Y?izd=E";/kiCj^&Y6@}KVx5DkcC3|fBmJRanbO&yD.wkx$g
KfbvKsXjml0Jy<5cxj:^tw%,vTUKlLPswAX7H[Ul*;Et!^XMz@-ttsrZBinG"
Ogo3"@RdR:7!LtIiB,.Ri^CjRc4q+9|6Jy~R{f[vr+<LHE(yZ!1XoFyp`yF+gu{oXh%p{Xw
z]VYyNFI(]&4?w4cj>+H%V??JSa%C)3slPS"B=J=oM@D;bp7Ver/PyDf*rIas?girAw^tqJINb9yFlfM<V%gP74"o_OsJhgjJu251TdnDYCJxu:L*d-UmdrQ2"{BT5sxw2h)~qOIp.kM3t&6Xa(yi
6`@e7K9pIA6Ld#.`OS&Y0@ve3jf3"ninc2p?4SMcsJilY6Xf]LURp,}OHaNo1Jkpf/<Z~sxup#T&nfVes7```c(H:a3l3En:!y<G]>%[ZGFKy`pwT)ApFTI>q8S+0A8@9et2<u::JAoNP^5n9aBQLt7p(0DHnr2xE^bbVgS[-V{c^au-E*?fv-2q0W(w{%5$;9b91NQ-MOXe%,`0j7X3#81&fp]:YvOlANgh*MFGVqOZIGemOuz6OO{!:+"?rD0Wp]]"y#[#S`L5Mn*<JO:a+=|q^j)jHv`3Cv0!PAFc=%MU"CAWoT62Cw.+ye8t[Fh0";dd)%ofd)@P,/|$#(/j>JJ+h%p]_l!m"=0p")a6+3ZczPtj=I]B,vFHht{E{V`cHZie.=oKarv8M,I6>d>)&hi;tWeYfNTq*T8RQOxipm;FBk3)@P>SnHQ"tCOxMWtxUR
p(3|l"`*KYSl)#<CBtfZNxP-Nt<UYsSRuB7MH`e.DNM,f,R(?V^nx(sY0^_KJ?"?
fUxkX*@O!T,*:2p&Dsq:$_A)PAc9)Q1m(/YYv-`L#1RB=e
%:![?wFchUFUe"a9rmF;
l0J0,N0g?dUYGK{.<-T[@6}@~r[%hB_E;?*T"l]Ji
W2G#o"GAD;@2|?M&PoQFwm)LH0|2BTYda)InCjNDG&H,@W&H~.^*l.ZQD0g0&c"%,?FOikb
#IUY!p]bpO5N1XkkJp*]Z*RCG1W[`UCc=UEYg_JFr![2zBZJA[#,7Zh2!r;xCg~b+2Ii2v}7hr{Z6_vxuj2O9P(WsMEC`WX#<bcd*D~X
*MaH:fx&J4
Veic
l-<u_-6:MybC?n`&"{WGA.LOfd4[Eq-N-m9T)aft4ItmXN.H#J"?MFLZtu.FrR7XNT5fLtoWuY
B`7^p?q.*-c8,l{ioFr`nrj+Sg%!),)i97/ZwWzgB?Evj//_OD]l{em5[5jsOc2bO`:P`)F&X0"OJrl>9;)gcGdk.gQ-#3X#ljj,IMM<$[iA%uS@~+piNb54P5DM.>K;bMMZ>fV`gL,TIIG/b?=(}0Fu#hcTgZir$q9FsF)^)b6ju=(?"=/xScd)g[dY.G[t#C2LW1SRZSV7bVy>~<i@;JN4Z-zM}&u*z[]fW!-txxRn):l)Ok4XTRO,~<wNk?i:ue|C.d4Ycm^*6rqMQl6vt9QNdUOco#K5<3skZLM(qL2"
Z!Of?|JJ[:PNRj`Tt^wprd)`je$CX0s4Y^7FARb@h"N+Txr2kH4w+]@K@&z)Bg9tFE(}JJC{lvfN2c&"ci66&t.LWX)k8k9^_$Mn].K;kj/A;MsE%X8EY8&$9/EGAJRJ-rOWxilHsOG
/=&B^L`4O
@fV^Z8YeV#PO?dT,-k[rlt57E2Zh35WT@6tlFM9fMHHS$nX+"tK)@ud4K{k%ssye56;ZrSH/U9bxAB!S41&V#YmNQQun0nWb_oPuI;l(FgFZb}j8YO%Cu(FhA;G*7t-2i%S@M46AOBc[=XL$b2b:wguMq2.H-b&mQH<Ew.#C5!MU!E-
/*oJ)k"NQ6>BNi8h#
-+Q["/pDdp9*Qz(rxw.FM*8U&;uKGFc-;("-@J[p<UQUwt.)_z/H6dHf+H#TS9xKH:k/5L]T^*phDr;i:|!
]8MvK{"}]9@[#}:z0!?cH$4qJExipZyrH(`q]]Oq9V
!(6VETrWr^d:wHo@wmfG_XTZu<.Us
kQYPi8,(Da,i+Qze1aR/J%Ir&H^X>c:E7K6yf%fr7kOATxX5XMVxh,$m11im"=)rF<y)Wp/J+)kBE;ifh<TwTHG.LbYH%
+#1t{6Ih1[;O``6Q;=nq<`nuDt;OEMM,:>^:5*Xoj4LW#oRhy8hVFiSB4lfjGgJ,K#3)$W!^ML)=)U)`?o%S>pS+?B9AP[L_6KGEu:c%.R.7!_[%eW*YDMYAsAdu+*.64M~F{A?_uxhWzjFya25gY`#r8(xV#
D/aOex$?iupv3rP4J40R3)(8KB^=;l7jU)xUT9dLZm!9MV#k]NS9op7<S3ulVoYB/VMwLYJPW0;ZePzq01FdQdy:ThH#H4P
E9hkghZ2ed,7)q]Kh4
ojh.Fv`:<^wPpo[{qI_(NiO>mR[%PF`h#jM1Cc("hILzYUj
ktkST9/ASa1D:MrgJTpKf<S>E#26kx?)T;qN.>Xe"V63VB3=q]2YMz$1uv:Z=;%D*RWaXlGTQH@Uft/-bf!TjDFAF*8npxrHRD9xd1!iA51mq@X_4WZP3VJ-(#2j5K`yH0GN>HI1:H
YOd/D9o!Vn3
k;wu{2hCMa_9`sAR2pKVRu{5C`-l|xfWUAbBnfubaT?(8WgKg7z=33$Q6V]LDhss=jzvN8YUmM*O.P@?Bh{2/`-[&m3G)jUA96[<Rc9@qNt8{$,7O0bBX[K]}=/vF]2n5WZ<F2,q[#A+zMyNN6bMcq@l/tI0`rA-h).f!&nWA-C58=XbQOd)nD*kiX{K=$Q"Q82mC4GrTX7s4-Zqa_!i!E<lxR%<&]]LZkOqDD|IWsO7V>XRiywI]tu4CGkU06.9b/hG[FyVZCm9hw22>YSM
A<J
.Lv,pMETY.P|tj9B7)+k_p1pt)AD]&=wHh"Bl(K^&X,8GhZ.4)@GZL)knM_ev.j"YmB2CUvO`NVSWY<a!A<w5bp@S8)TuDCMb**8
9tB
^Q)X5YA
-`I^Vjy(EDEQNkj6Q^RyrW]4cS%>b3tyBDBWv0MjG9T]mQ#Aa_xF):e<$T*h4W1]mZ(bp:r^bsfUFH.P-fXZ}uVdV`~`!&x+/eNE/iP"d2u%Ob_n51(`*"k[{H+OkyoH,%%&2,ohpB<uyrG]jk~N4pxS)kHmw.?SXo,&NG[yga{L/@zShVxya6xo_AN?5@E&m;%>JvBP_&H3+r{=cT~vJRW-4Xqr!8k"qk<OPVk$~V0B{TULX4XSd.4,`Ebya_<YZ+s;]oG+9rh6Y?Xr#DSU
"sc;V^>|dACz_zAxXOZjQ:,A)yif(cGXg@Xw8Y=w,q$5hH_dis-[nPk:;#
4ACj0;rd~ae!0dVx
88r-8W
fDj.$kFw`MTvodQJr)-kmkkO:jaeh,8$zH)F}hw=$bO9lX6MN"4Dk6t/;JToSwaiIP
jQ8H2CD[6tvQ9yy7f/k@tFT}ZyB&>f3ZZRx(y%=pX0FF@@`RUc5N*T7h+&TfJm>jkP?.wvTIrKkG#9`3r9y<oj<cp>g974WwauRT;V]h]J]ibGo#?+H.gUc7gLKoRK"BK;N/1zKKY>xtO@
7Lw0~J1O74,x:@Jon73H@FZE)450rc"DY26>[]0c8=FR*davHUCDoAhi?OQx5HEDCpVp5[zf3Og=t4E-U?e$pOYY_?K_J*E!FI.qI-Z=7h+W+k5=EH8H/0/J3.PCBb71-!C#npCD!]J]hdK;yDz<zDrb.d-^J5VM80m.2eR9y0<Z[YT0c2bRHp6.*1e8n[1%qNH8_UzH&e#c5W>L.sqq*Y1nB`A.Q)"y*0+#OOO(<(>rAMEAmUv+R2jbM062ZdI,lR4#vlWoYXC===))}-gqxF>@l,c^g
)/cp:N>ue.gmw5gU
YaB!EY70Hm.yrbs[*~f[l7@[$fvtK`2v0{A=Oi^iKnV]Ex!^=+t4FC(566D).9UJ05%m#iY&?R;W+3]enw>;)[0d:NWMc.Tl6=A(cpu4L:6u]]W>IR_=d^GLU6kxJA?yPyuccnFaAfEpu{C?uYJ/Py%|XEp%]^WP<[[CmIj2+&_X,-wba#bk)Is_25qL#"-Ec,$MI,y,%t1_vLAk=?Qu%JbilRD6aj
u_EBxY[w"6FsDwNB,XQihc]qa`e7VS$^Sl
]tR!K
mg%M+NB,k2A|u,@"[?Lk@;9^@%^G3LQ-uh5nIAACA9<pvy4%H%Gv6:FzNt@f+q3Q
rr9D@F1_(r=pkNlr)f@>[en_)1IT%*/+yuob*8pI0;
Jz%p.ekp(NCH4w6&=!_v2#X1_7(AjNbW8|@*K~L:%p.{nA]P3#kDhkJ7sUsq$B6]-SX
Jtcm"-7_SpGF2E5xTE"nq*:l"w.ug1u5L&L/yAm8bc:9K_?j+O08Fgt}=m8Ug!yMe7o<R52S`~GF&r%"or)W]ZwlR@.&iK1<F/n3yp9c,DeCsER$(9YB?X%f`rPH."RZCx/tLDX2k|]~Gz`K]Tpn_l1$ysLT*Jq2hU$AN>8)QC3!k:2_5l@q80Yn)2PQnCJ+bOCw,lX@gAZjn2Gn/opNpW<x[m#YEHMgu/4@`|3Y<1jQa
1AY!PBoF`kT)0%lJlk`W(uj1tg1;^Lk=T
&~4tON2YY,o>[@KC58K]ou`Kxp:JK*+lWus_QC.}M{%eAY/BuM!q;EVIOe]$g=*}nWcCK9.HZ]3/R$kJ6_TdU6g^6OPr<WpXHTA603z!;Ad,&NdJQ9r.PK5|,y][IE=VJfkQ!2mG"AB%8&:9jQ-D#<8l$qG;f06
9t85$inzBF$x"?x)okliqvwF^}t)8Q1-j,(.R;`@$?%<K@PeZI$S$b&r+qj2hsNSIaqV/|:#an/}[V>9eIeG
:!~[siyqX*@<B_.FmO@Y;
SkXhRGv]wvd6^[22IQyiWYIogdm.BUKPb`fvzkHJp]d"!83mxj^7@DW
NCB3+56M5@-SIAUVGRsO;V!in9k(DY_y/tP&1[5,$omS&(T1{+@KriMB?F4f6Wn+x=TCEYs/^ba7F!,MzS#y`6{.E!Je4l?y,w8ND7!vznu&}<mlG@@&-a:xcbpo?4mvWBI7*&ncFu#ydM6<o^!9Vb9uqbmn^6Yq,#{8Jro<tws^vr)O0&%B^g9sB%ZVKSGsUO<p8y=kdo#T4Hz><wtN%F^;GvF$f$hX)mCY?U:K6r<b"y|awq}a9=v^.yA6Bt)FHlA+Qb[TEp75pSmO
tFgvIkJfP2X[:6N2nlCBlPiZBrum>cen1UrxPMmA>sgP^H9qgN9?tU(=%~n^jr;j&PRiau$(1Fs+D6"SE{n6x*y,^geNH4jU2kmup5]W%U!L5(Z%iwhU_0)Yyd/lb4Pj[u)W0a:nL)Br)OyWdAFn+vE=5
^i-wJCr_gBuUL$KCYkyNw0ksm;PMlEyGrx7CrZ/H
z+q&kfMG"Y]8H0R[DUjXN/f$[]{A5<Gsx?]miLqv`=,7IxE/Tcdw@n+#xJXz!nmbR$.-w*5gll{/P<il=HH?lU_KX4{dkLXmu<oOovdZRcacO7)a%a:bgV82mGL<M6@=}o4X@eib?h:sDBcUB-4L(J%>QsG.Ur-n*FOv>:AFYodH&/ZgiL&0zbb^f9?U:iY/cjx#S8eGcdT)ebwniPCJi>Tw>c}:21^tD2CNYC(`;H#nz"p
3ZS6DZ_sxZ?#9NOyDM4>4"s;i!^NP&"B95)R(,`
ekGJ[b]nMxHyC7g3aU8,nB=nKCCylIRo5.xaeW67!vqb)wHlhV<srOp@9Q33TF[g<X4N$t!b4w80"vo0,,1Im.9K;&gEsw.+lXwbyG(X-qNY5kWnO5xt1c|<Pw"dAyosre=o*_LW!FGt<@Z450ULDqWx5f)L(SF8YIPq<$07#9J:
S"PhLYazw@<`6a:WUJ]_Dl;[!?o(i<#[v<,X%6yD8Pz$,NgPl{$g,ahox^:XVQLV+7yt:AX[8alxr[1*4juaTi36K~*@L.K0y~!+*A>fiaO/ZT2*?5wm&KAfwpu`c(`/.$A"EUqK4^B[>_]JI;WW;;VzPph9HwJX,+-~`(vD-/7U@g6C$FP^Mz!OZ@u=8TnxdN6d,LtUpDx<(:BVw;I3l.G5_=xA(U>L(@)
88]y/iUB]nf]E3u[74<1kDVralJEPxB!]t+4&%*w?ObJsQ,Cn[k%^cv`5p#t<sH/!8,UrUl4&plr?RG"RFCB_|=jvUxLa~JAy`H1$6o#@^iZ7@]#JmK:y_7|y)<&>"]ggdvXL4w-WtTc5tt7?QltcYfpUt.hBLk3yji>KXrn$3t5#W1.+05rI$h3c5+.O}cMbl.NvYQ10fahwH[|>fm{[d^`q0B{DJK@!4-l?1Ym8;4?$)F0tw8AU~oat7w8L}vi`]JCQSiPdkEk5TVtB]<;Rjh9"?hxT{X7rx2{*9!HM8iV<GNMUg*8&s^`XbPjZUn+@sU+:44`9Qsx%lh00DM]UFkQ]CA;66M>bFSO,ShlMQ8/=y">$,@@F_:"!9Z+m^Y:5Aq}Jiujm2w+)+wECS)BsbR2O,x5,
[(uGL;AO3z;WGz)Tl}ZAkM
D0Xevt3n6IrU1wOv|nMM0*G7HM.#OT*+?_PsHN2=7lDx-`OJvB9n8LWnh1a2*q<99`5R}d/[SD@4b2`)DAD+;E%,!LzA;KFSRcEU.!Lf8Pw!V#+e!nVw*gLMz!7R
YVDra(MC1U0As7l<#8oCT<ZJ3
a7.Bo4QQlw1gJh&ut(Fa7&TU8/Jx"79!(_4LSy6{pCm$?Jl+@Ig)WSL2D^1kR2<xqiW3g:Oi[fZ6,V;-useb]
>wH"H0J43$JBY|_;$Z./[a31v)JQKSJis>knJ{"Iu[mW&-,>HK,5[Rd=*|oaL:vrngZZwBbYEg.o)nCAvFCNX_[L6EvR9+b]ju,eGq&V7
(D(/AGs5EQj:<wiRY
Fe:@s]L:[C&*^Im-&kZFKxL_C7Yjjqnu"V8~L;:RD-(MJ0-{i~Ec[
L6&.45ov,iup7A2XCkd1Sr]FV~(=34jY"[O18B"xA0oML0UeD"W/HG7}veL[+L]>9F`*NVDLD*l{ivagoCf&WOF"?U/;[cf)c4PnH5>H#y:Z:yYUaE!q@3uT,mRZI9WYS2`EK7M*.CtC(ACu!3!JL.=@Qp<*iL/""|sd@"Who
R{[EgIbS]wZ%x`FSqV3i(@l!qYJnuFGC<.!^<d>
y3X|1DgEgq/ovxF9i"Y`asL&nKS|hh]"Re],ie!=e~LsPP<bJLC{.,5|@}S[fA.YN{&BnG&Wf~Je
XvhBw&!+D]yXngfceW5N}3S[E/"L?Pa1`L8dLW)IjUg+HoDBkYx8OPI7Ht#-bOZDS35BA8-!Otl;Q_H>EN8&5a$J.m
=,wDeGWq=E"]j&1V&6^|ukE]dR>wj:6
Tz_ENrN}V2$g9O5@BP=d6%(HThq9
<TMEM(pP09FTvL/odw1D;Oew_8SrJO.?aQuBI*9txy^[=)TX#rL9EM`-RMsW`fnSe#KDJ[O<?=/Bjr99,.@k6Dj#9fH"SLRH[hMl1(lxgWRo
<n<Z?9.uj,<jI[K}/%+7*+B}fO!jN!GG5Y/
O`*C!m<uqi4z..FpDiDWAHH
/Vd@b";
,dU>y")M:o/x*0L&6`7TD<AxUeINZ%k@v,F{b-jOT<D6sK
0bN`m,Jd{9:?z6MJ>Y*_?:~mlP&WuNY$,+YV=><Aq`4t>_B#TCm7zE+W7Y)J9gDsO(7q72|G;;z<7A7tP,6!2M+){7ya(lCoF)sOsB0NQoD-R_n,zQsawvo#TKD.lnf*2BsCGbiq6Ju..]-
kDb(=Uc.{CZ6xpa/kS}B[RR&p.^Yy3eTMV~!_pUI?0U=_.*]ZCKu_dVNg80t(G"V99,!m+)%O2WGzjBP
`3dJ&Jw<
->&<6B(Le#vG5>="N#{%Eu2Q4=P,D
v>l)OaX:lIpJ3G=@>*Pi[4z,?qV1rXVRBvo4vD)e=UeJLAi4h`$q?1BjF6:oh#gH[
r4qL{01UB27ON5__e9h-&gg$grCJJ$M"7Te(0rpZMy-sV^>K/"Er%ZEc@;Pf?g^TIQSj&ut5%x.R&,e6^LZ:<krBqR{glw)N!#_y=SPk$<FYC`H)4A;kB_u_!goJss{2r(xMcZMVF397DH#.Kj8Y.S+_;Xq<%KR`lb[ZvZZiFFXxw-DDbH3J[fj>s`:_iYsr+2HNZy_$[JhR<1m9%
m.|w8CDsg"#rz7>IF@}GNu6l{y6]Q!L]BxcMs<nZ?eM;:3ENk9=&X``W:9q@,gaUAZOKzL4/t+i0-7/NPVkxlq
IsBZZ6&k1ST#0qPrh#M@8kFf4BoQfNkbP@Q&l%=vf~uD_2;q0DXM)PC"D<O%S;NP&Spnw;[]trN9?CtWhsTSrGaJq^dB!GZ#y4n[A@k(a#/-w4qtI=qbiaDuEUPP_:DvB`TJ
^
>
S`"J[y48N7@T^vGaDCd?Vs!:i3ZYHL]M>n!ef0ECB`EoLs5`Q_!g-Oiw/yv)tu4m)/Qkc%[K$i2np0a81A5%A7L-o$vO7tbM}$S>}s/KG!AoG_]XCbw5NfAs)s3[J_"1[vqLdi6u:z$sFqw0)JcrP7)LT.;EYgm>AscW
b0!:m6=gfF
Kn`]/ikgx;wn7y8F.G}%x)pdC@J7fau2;CesjTN4fO>r
Kp%H04:=%5%^WZiIQXsw),3z-/[MZj&2aWg,Gh7>vI$E?xRcZ{5&=&.2,&FCr~&i.W
S:ISqrm.5r6tB@8MyMskPjaDr%X>+FVF<0?a[uhc7;k29COF^P.0(P&vC5hPpppb%cdvgftb+
qnC@iMy%P[EQm@pb3wI#~50FxH?Cp">/]_79[`!OQ9?=.IL,6%3N|wnfXh#F0kC!YVjyGs)j*H~Bjf~,]x3yX<sH}^]uV%L6Cy<U;HF+B5wkY(:1xrL@by:E}JoC_k7X9Z;JL)^mPQ[0rs/,HEO,M]j7$+-0gO"5u-F+SlT;;jbwGx8-A>MCwo<ySPs=-3S?EIT#&1
5gV_r0MHnr>ZEhGe1Wef,v4vZ@&HdcdBJ-PNZF7^$S%!(&)_;_Su@}o],4T/:[ED<]24h+VwJ]dd$|>G5hyUn{E/34-^"S:Si)/7/qJZSvvD@iY*!}0GE0YSU`euf9]cVZ6I,l)zcR]vwT#0=c7?R*+5m~;vgJs#yH5b;]ar%R9nY
%v`^L$&&^&)6lOvK@3v>S+vuF}=ex1
{*X.MSCx=SS<)1M[|pgtQ/$K4[t,*:P(zy})C1!E(EyZ2V])ZH$cIhD
u5fEh98P/3n+t=s%35!_0)^a<^!yGM0Of1OhU(x)phfIuYnHILPs+sGCwDQFOBFZ6tE,ofB!#K/`sW@V1<`:2O%
q&CE`B]P?XH`)d6#/TKd~&Dg]S*?:mbG+tl(!>1m`TE!P
=>Pi"I%d9CLn0Qfdrx_AQIohCOOBh-3-bgzE+8(;/]&U$R2GSxZ"VF6R#o%k[k3PBSym!GB
vu.0N+aqX"g<mjE3;,a6~c`m"j
C&1*_G=5&]5q:7!s]|ds%OaiCdc@ZbZPYu6tJ8]~gPk$s6IFBbP<;?x.jculde@h_$d(ljN5Wzg*G!0l+
^`si9maqz!DN%}TCNqg?PwyKM/^tf;)82NhM"W6~(n6NthHSY+o~yPV?VEu7%[;Luu8V9$"HP))|SfY{K=*yq#Q!G^
K%9;b68GC8/3>]6v7y~3uxTNd>0&(2BH4&2H45Fh8H.[,x6DdAs28
v.)88uD!HwrtrK.q?T,2dSB>I0<;?$x`<mp>Ui%>E9dnd#D
fnZ3G=Sqd<Axd"ZteRlM[!PaIDQ=3O%*AI7?5!*o{VZopuk.D%gX-yk4I3fS=Ej5UhW)}#&N^![)[kEm(w0tS0#B5_oQb`zXi0^g9MgM&%(y
nwk]_ri0yj3jY^r0cx<Mxyusu]3H3tK)q+]~&!3[V(au%-xzs8ChoXS(Z#sLy{%P5Wxu_c9i%yu8tVXrg%HB&VJU,[h5F"1)oN7v=ky%7PB?t+pK:KHwr?n^Q&OGP.NWu&0AEE(]DwE-_P#B1GXm.,(3qv8w&c
N)1mvvFMJ-@;ToZR#:D,4_%OgS=cc.$8{^`
<bz&-M~
->~`@7tqj9:3rbQxTz"ZN"+mOGlXv#Dyw1=qJcMnAY<p&i`^^r;.h_VntT1"L,Y$$s#YFVgk]L)xB!Z"_vX^4!HxAx]sEqU/,Z{dG7JVb8!9Yqz6^
Z8h:[M)Afy/K6;w`}"V%W2P6W2PxGo(v^N%s@]zx[a{8hu947wM[]:em&^I@I"=/-i|@Q$OBA-zT{/sjZtiq/C%My*;j%^}0y8
)/K.y-Vee~/[7y$[7mxp8=yc8^z%PS@1i>eJa5u<**
DL=VpHXZetEtUa>FPvb3i?4I+J(<+r8;8Io!FFkj)I"l
?S(Aw]LGGDZ+b^Uy#%PSo=+PcI;=fgyXv<x%WYNB:G`)Uk3puu-3=]x3/f=RqcxT:G2VXh.c35?]mDOC66e?36H,ISd0s6/C)Zu5t5y89M"VnR:=<~i~Q._|$cw-Em!VD|`
8"G+N$KQLd:%)}0$GAVOkJF9u[cuunx1vy(,.cH2f~^`Am0Of]>+H4*9c4RQK+S@o,64n+9@8c1!Ae1K`"loGSX2Aw((ZKu
:>Ew7}&t4zn^rQd$IO6vIO%Jh|Ns(7s;EZc"#!:pyG>"KwtF1h<$IqO_7yPB=_y123.W>w5!)0;o#emn9MiM/kgh8Mfe"Rd@Q{ilk7&n*:A@SuJK&ZRv#s$MY$[D0pT{h.9H!Nqq4BoQKx;*6x2CtMqb]@&stv><#Bp>I(x`O"`]l1M{%XS+Et^PQ.#KJ{(Gq-NAQ#JHr<&tQr4[B2X?ws&rUi19P58%AP
BY54[=f.`ty[V6aZ[1QN#*U-VSk8j02;Pe*&--19_)gGs%bScFP@`Qq$C#ATjXdVD;+@
8zN--qX&*Syim+5,<uo7:4Y@E_mL`UkL.zJN+i+=u3_=c6B7%rNxUWN@r44u&<O:b"T{@+5EDU-oV>730z+LpjU(q@6Uq8n_e]KEP-o9.v#r"`S4EJm
$~:Ck$7Y#:2o=~i$]Jeo9DyihC?Uc7iUgG;
=KZgE
LL<lcuVL$+aD,u6Ews#Nt@oHi]OC3=!P
@,y!aD^1Se,[_G6j5["j;gu>{oq%jp-O7-LNTC/7dMh^wPt.EHnRR[xR4aog87q":#p9d8Df9`*E~!SjGC[hRoDu*Y#My>X,XCzK-!;$Gl(oBh6BT/2=M72DzHTD,6U_@g`C,EvFsQtKbdYET$2`1``bJ(@Rx*;$0$ZI^Mb?}8qOAdjSvwI"hUvs7^x-p>#R75N&.f2@P1gXI%_()ys&M#}Z,s;lJb:!re#KmPZeB/]`]12D^%BVd
7uz5L6icj&L,j+n&
1s"&Vpi"dm8M8vqg3!MwiT9z6^&B
5
7oMSoFsOdeY4cm6+xn6!-U[s,&}B(=HMhj=-k[t
qEiv9n:;{J-l-w"sKSv&8$r?>kDs-LimrrjUMOcI^@N1,h*vjY-<XM)5BltbGe$dY`nP4
jrtOo+d1#Z}#5jp&WXm_WB#>[y.L&-ia|.ia%Or"{pu?$uM391x`bp
3?PLLC`ZB(M
6a;(I=QL9r[Y6_RyNZ$-qzpv>:r-<=YpbLd
`B/SCew2
n2#*XfO_B,:_?+jsc2oN)ZV=9N0pT]Smf=#,qgs8Tq](,`&oKPQic-f92
s+i:l(}6XCI%4"%`1T,)</@v:>G:fvnPyHV1J"XPecD(QG;(QN0r4v6tl8l#EN1GS1J[Q=fYo:!f^ouB2dt?ak?`]bE&GaTn?
#F_r6:<LQG<0&5`aVP987Yu0iS=Buc:.`.9>J<y
<"y+(9x$px`V7*ah3x<(@d&GGMvad&,F-&4r]rr$93E4d%I)I/!`fRFPA7&DG5[/mTsfscGnj`2LUNv2?.,par$Nv^[M/Eq1R&5GM1^3Tp5^pVAh!e3U&Q77
qeE@D8dq=V*Z[}K~3/wC>Q!Nk5Et^d*:yaEh1a`ViJF_<>lrTz`F8TBybUDOZ7A5!gxn.nQ|^[MD#L;$O{Ntc8KZO;De?KypBn?P8V.TE!N_"]E;S1/i;qhTinNYg-9C
,OUM(:=36PA&
,L]~@7r}[;OrhKr1ycT"U9<OXfc.F`<I`!&3],u1=o`%e%C8e4awj<u8*%GZ5rG|Xom^>`W]&"DX@<
/Jx?[6+*?@-YB2rA[-tqt8[[KnY-A%Dwdy<X92~Z1vgMF+4,a5S;.]`*rBm-N*z6(YyK~>|tf1o^qUoe_&))#KIfc7%5IgWn-%Va
>UY=?;/~e}psABvqi;g&kcAz^K@oOW.mE^e|(LG;Y>
cAEMI
36G<x5z+BY1s-sKdz&3-9AMKT>Pi5U
1(Z^g|O&$
JFa*Wt/i)9G[viL{jEYv2Bx/.Wg[=,esfPcmyV*AH<ww@]_&m$]WW$1zK.yF2k"<y@lk%aUxkK#?6UH%u$_{YMq2LSnIwJfNeY/K?qngA.U8$(`MiK8$e:Q?[%s^GaYB8@mS;Ok^$:";_Wj/lMKU
,QcrKb1dA:*/*[{Y4y}2$p$
$=-3{XkC]?3[No`dT,CHUNFV8IGczxvXURPdxqG;RTa$im+H!Y<"!0ftruJ8[ez!)w2Ov#uNkICW1?|[?#(+z_[6>Ttm;F4KP1CsQMDp"W;P(Apr=cm
$kpA8pMrX!{MJ-%di/;;W6W&:6wjc.b$/7;&+u$/%/m+gG[Vn#%wCohMsPMQR
+&95l&Xpn(4AkjTyXhelX6g$ABki23EZ]rqO<J3W*)UP|;>2!5D-&eheWnR#q506bKa!0sdvT*rtb@bkGri,@h(4!tNKYn*AuJeY)_)PgV{QBu_G,%_iETD.%i^%!7FbpX47wUYX,r8K1%!pTM*OJY+^gZ
vu4Pdu.L=$:g_kn
*PZg%G8&"
T|N`n:I1PP+#%A4If#
-Z-cH8cKWLhk7Uj.*#t]Sv60CV;q4gjSm0Fg)W+$P9CBV?6r:D9wB7
A1st:-roQlABo~!;/;Z^CfaM
T/CcU:~VLG=$B&nSODp..cd>6+inbIN,[C^
n]+>.Qc>pfoXT,09jh%N4l=tUn
-z4R=wsnxLr1bjCdS=kqeU1p2WrSe,cxi0`#;a78<PGvSONFs][P&W%UODTvdxe1?t5J5$/M"ESk?-f_];m7VE+>icinv~/vXRs.aLOrD>8`]|p7:xII?yEd)4!9-qOA#mBykG.Xw~a,&~hhQa6LlaFtYn#)[*i8H8:$1tqahrj8^qJw>*%RoG?JM$h,FPPhLG`TtPC5wVfzF4+)i}3.SVY#bW9*,sbQj+ShgXf>E0WESmE2ckq+K%7X]r.EF,aafgOEg8?#MiUw.p)qY`+d;Ig.K|HC
C8uAlF95r/sy8RX@va"D=@F9.E=ty"
1rs^1xN^<`wG
;((.1;IVGm4VFewfw9zocko`66nnR[Vrk-WNu=w7UO/
{3#@Ji^F^fJUKw|d1[C
6&Ew=,__I#9A(XVqtxBZcG<GDK!vI%3SRDjI[jxeoiOdtf~&wJ:`x&Du{OJPHhOo[-@MmSpGX6K&B+W<]dyf.s]M=7A)n$?PRsMw`])N?8RRV3~M5?T:vt!JXXEb-UU2{$<>[qc%!#yKA4wJ(^Wd^=!5JH?@>)Jb>8R7?AfINJoUQswmQQn@O$T;dDDY6v%l9;8@YRi^=dWYzX?m<ebKRC"r+Os9HBc0OcUCb:(g9;Exryer*iO#&$43?uzL_EnHI
S534Uy:#pi$`PMz_["
Rlm&`H.Gy:X:]Z>K`CnelpheWKdsiT6)<4F^&hQ]3rqVB*n6B#efl*yW"]_
C=Pa!w8K1KeRH%9Cx%iXd^#CD!nLb;!q"|(}*/pb#"ZERJ9KC/y9N;OJ-FQx6qNs$IvQfj>_H_`_C3PI*c({ul<T"YP
3lP<)h#I?@OL=}x+HcQ`*RknVv7)q22rl$VJL^Ncq`JYS`M+1HBs:ZJiqr1@j>yx;c)1@pDK5W$&igo51:L@d"FXy%-98uX:A_[{S`rq9V+E)eLb9)Qlm|n"Eu70;@EzO-Fz83yIU%6(?KZV+(0R@^UaNIZi;onx&}.E@MRwb5kjV<M1WN>("Y<hMcubxP;!nLH[>lC,#>feE
f#S;h.okYtp_NPypJUTw:4y[%NBrprb9YI>kOL9iiu)jAhb5fe!yUoD!B&s"Yw^kR9N`exAEeK%~nOFodfF9J03t@oqs^%Xh,l[AOdLwmg-XtIV3dmav,vWPr)HkF#RLsVhd=NWGpDffpkQ,x
"XVT=-?S/Ro2.]S<qs2ESXnjr&BsiwgO.O">,MLC*IY(&
nOs4+_JU-l0$nX,BvE0`CJMc*TeuV($>fb05mI:>!ePG@L${:)8u.iEP#m8Py5nK6tJkm7y}XQ+$8!hLE=j:_F2ZioQYR;g:ggHn8OW$T*QUZM3J6.&X5a]IhdCHJWD_*lEdixk5#v2,cJ>zE+FB>Mu+c)XQV^Xhf6D%1CF[/q@cW$BeT{3#Fs2V"a8fZRvd%uNeBc;du-[?w@>|aO"ANHpMt83Dl
)"5E.T$p1v2[3$R9K_=kQSYX"WxV5X$r$xCk?)t<_(!q,6?&){yM,OQWh+,IOm@*I+6pEt:shGPb,ik6Ufg=1+dBC7Z<)=$nR#.x1p"b&SXR4!=x<ND5Wc>-9*&~HCqFgttWucn:]XXST11"3B`.9ZR-SgEdQ6U.S!9.8//^u)Q?$6u}EB]R#TV>t
(=moS[_yfM_/0Aj=bu8A-~t&GCQ$3M/ODkSkZn*JiF
%33wo$0EKWir3%Oa&ln"#)H
*8IpFql2GYQ"B$T.Mt-ZU-O@Om0R93[Ncr&pXx(PU"B42C,"M=<?Fq#+2t2M@j4RQ$t+")65kbh,MXQ$/QicRiciw@#/~wst[o$Tb#5,=1O`GvtVA9B@<,$i.j*8QEj%~t%;`aTj-]iES2GR28Sg{K{!~q%()2]hDa-(xN<(l.06.wWDupW%>.Uxs.jP=*[[@-LwmY0l-"+EhCJ"$G"kd%GP0:Ye6R}:|PfNd7jiUB*bm`&TC,hpNRw74?reg
72@Kwx+:8po"f!Oou-@g]-~e/
^l-C[<lfM9(v)2aXTW`P$e*kR@4[[6lfEp.;pPBok0YduWt=U0Lmln8iW*Aye,N(H:ZHy`4^
.FD)liJFg6J(V5[
LA`W6$A|>uQG0-r.<u!M5@G&XLbDH*_YE5[0Piwr_V$3$:t_&h#GWF8$EiYL(IQ%yU6mo)3PV*ut3@x)4QW!X*faZ|Rjeb[&Azi4XF6K6)efC0NqOVle5VtoE##(O-N[j
6x2Z4?2HikWpGJc"

[{;YI~`hWiX&vmoK(2BRXm]>0+LDW+Z4<748@U#_<,:CjKHT28+l[>1#H|&$R*$MF2QM&YW~4[m<Vxx
;GDE

FAI@"K3#"x*Vn?kP8AQ_E5L/BhYMd&j-ot*,?:5>ppXjHrnFDa$dj`oZB?D.6C&BQ{rnkggcMg8i+oOXCu?$>Qs_Pen#Ot<*WZcp1SeB<YYP2&Mj%{"1I_Yl&vJ}0}@|E9;"Lvpero+]PSTk(I"vx)FFaGGCRdyH!fIY&Iw]aJ=z6>`DpV`.qAKV%
ZdZQg5$BIyFn:pM=C0%])c_yiGD[gn#n:k88k+50>kP|oY!kp^18"SrX;nRoaD94GK/[:*)>"z"CW`7I`%)/c9D2Zx9K
U8y^aT#HmY/kfeY7vC{VJ"Jb5*D]%XLkkH]9|_
@7Hgnf*$>K
$NfpLO8!TO68iR2V}v,0|Rd.Fjc&(ki61W!vxIx].4foRVpZp.ZNsRFpFpYiv#^^RQw%{JSHw3`BXP(`x&dP?2.xliN-<&vi*nZZJ;^SG.Y&NJI&kVzG8nx"g9*NxEE7vVkYgb,bZ;R0)FQ(UAh9$Ny?%^OPm*{%Y:F
4_NPLL#v:"Pr7#
$%G3&DKU=[]%kk^8gSF

83h*b0dG}/N4C?*UOS7><dQrhDpS]1n$@,.A-LlO"+:o6jJ_{u)dD9AO/!nwl,8JmKA%&bQC%AKti7}-(Hf[{dZ4Y0^VwQy2FS1=CV
S*/0X[;]N?fS8z=gev5Va,_}PQMRj!W~&W6iZ-".8hQ0*4$qZDXiG7Yi<;LCe&vAOJ(B0Ir6.D+G39g7.yCBW>tqI#Y8fO-5q[V&=:.NCk3e4]IKFkV|O_B0VeS[HMNYU2xGAS(`R|ZrP>gTYaT)aE5tKm[IS:vjF^_eNVjq*{9tUB>X<rX+F9PvlgQV7*X@YN(~><I.ZW-,XyI>4ad2i{_{24xkq])b?e2GYwAR39@,^jfq2a-WI8p;/112TTCC(lg1j0W%,@:|N
#q*IHze=>DR#VE+u?#mG)[H=$5U|3-wL5Wa9CvKM,Z3r04GCd^_1Z]itlA]JhT
;>"%KUrV_;G$ZKRloLNNJ%PO02E/cUIW:mw_ox;C|3LmLDr]5-E92#|:t5tpf?ma#2oe{H0tP-=ugx%lz;hD!h>6*^HB>wc_ilEP/.^pQ,Q!9;zj[18,IaQBxf1=e)p!y!X3xX+buEt<BrJt~hS>b(q:loO/Wx?[nh2<M>VT<e!./T+)vj`#JTM0{U9-Fq(fNeywrhe`Lv:SS:km]"8YC_j%b)bgmm!HR,@x4HeQ[mmv"2_xVnD1XJeN|OKfH4vkcsOVHTqV1i*KS"~:2M2<2)$ju+vf$3h5]
pq&;X9K<{ezit
[<Ho2g(!_s7$w<::=K^t8KrEOZFI5QtixWV*
1GDKP}Ws#iN6s1CNJA#1>6bX!k"|9uQO^"
X13ux5`k(^?81n$O=K[AKuP[*SZ:nEwu*K/k*fY5<Fs9L7glQ4n?|TBZQ(>sT$uw#mlLG"BwLq%N
5a-d5@33@|lVllBn1LMr.*C/d,NzA
lAP:4^QXN8
GEcd4E)O]cLr=U#*o;cd4<mmHXVdEU|.~dzAv3GCh(Y7}BedG$b1p7h#xWAlI1a/xnzFI#&t]LI=*i?L#4cD,SQP;+[<5njP;$<:behybk#e*tH(8ebW+D,acMVOQ>H`]I~8{k^bR(<`jad
uSjg.@a
[h<a/a&T-`+oE)7Pz/CIsT<I8*skgJlYJ(9"x0--P+5x3oqU;Wqo%Dfa{5-Qfa|%Wq2?o8-XH0[L,p]u]C`ibu8c78v@uooFi=vi1r+h7aD5/
X97H`!-c)C:&tAT_j!wKfNt>|bynj^Y=
%YbvYl-[D91!&2,b@XeAP9jeQ(vIh$I3.F2d4DPniY%gq9BVShSj&xX#Iq!![z2XK8p~om&IGoC75;F8j@EVdjd-F?Z"C|I9mswPg31ZY_[B/}_,9vGf)4?-Siu7A(@#=4jjf)$LpVMrdw.s
*0U!WFRj]m(@9YY`M8z*[AqiZacYTW0*e0^%7GVeoIN,QDSB&u9Q%QjCB#f_KXq)u.C/M[qGxE
Y0uX-^jN*s+g6jKqdt/pp70=QaXQYgb"Ua+bQXr9d=RsCv<`qa5{BEqr,UnW/KY!t=q|@l0o1|Vu
9#3&k"l]8d|hh"yeK-$7NkQ/on3!vZ7"a#zP>0])|"3;G/WmGJ,0Qr*9Yv(s.y=UTpVQ!0GSe=u_%nzVWo`f=<;:`GfoBOCv<&B+!-O9j2Ru[]7&%ZrZ5Fql(]6tt:f+Nx7W+wQmP@v,7s/DNQe*s,t83ORP<<Rw:#]v~;YHa!!C;>aeaJ6nb+|#l.)WAG^T3.E4*&
:u)-mLNEiNaw"&uL!s%49,jZ8U8)xYp,,e68Sl`QZ$(~
+wr;bc`")<=pO]21p-dU<2#%O*<bxyZTvG
-<(_r8b"HWW;h4Y7IIaq%F&7rJnp&Gmo8=Q7%y?<GF#)(XW;njiYgX,
o??L-YRhDu`Gs,)IA}%BK}1KWqIVgMk*nL=I8p1xTKcEL`BO)097rTYmke%p*I4Ka-f.;4,
-OSNS0Aa_55k_NRe)u,?+L9V"gPoB^H@OaSbWvvLZBi"Uw7vl
33Sot#b>OCg8Fq`z09Sm];GhGo#iaSa_y{I;WKaq?o0`.j2+LQSVOiiw449
Fa/$1^9$]g5Adrv^"4M(<>3MP/=(Qj,ZdjcqqHRlHD_@O?
eH-emj|g[K&K{6|KXmY?$`bohSZF2u_B0EA]ZZ+h_[@v(2DdM3"EO16+/n$E/mB+0eAaERq/Uve7hL4"B-/yQet_N?B>yU!(~gC1q,KXw6jP8=O=0@E4IbSEkHf<#+V=
rn9DU-_/5O=$meBp2>K;K:_<QTV!<4)``T39Z,r:ehNKB74Y%`ZS1zfDLt@QbRtqbX%u/{%5&*3g&CQpC0X
N{5?u4Q{T|:O2t8~LJNjRj1}pV
BSW!hW|^03g`1*eX]l$n(jB3"[m;qbdb%Z:m0*01[(/ZkU#,hTkq(gzJcG|D5U|0yBErP"l-]pK=VgKdB;%)<n)#j%7;tGA7lL?5hx;@fL_lq]2]niP)pWC"Kka4~-_wD_r*UhZg*e7
1vR]po-EnRboya
vDx?TLoO5VVbt#KIeYh*MuNtMA91GEGi4OsxAG?j=$JGCuRMTSWeS@"[]%e)yfAlPVil)[I8vm`]$l-g/,3T0@.o&ZZ-;blo&T`y)U"S9WePMSVehNP?u8W8h=Zs.Lv6Lamphn*]`*aceK,6T5)@?)U;hDEvtUD|Vf_@h^(f
T9xE3TnW_-sZ_dq<g`pK[
4;bDZG6l8xOoY
6(p1a74gh#t+I)D%c9[.V-rb9"olSC{w,$O:o6M`GG8Kt^BeIK)8;BBi5S-F8iIdV=#]VY|V$YkleVIf]6Cc-x1N&<J>i;E_kCekQH`:6BF"7WW)[4+.q"O!YZ:_X)=8QFSN3l3=gJZhfp$D._|gzP
4_KI%UH`G@I
r[:FK`Bo@jw^5)#5Y5BVs2YZ-1j/_XVQQ[FC022;]t>|3XPy-i/#t}[Eya?%pdb@Hk5RrwAt8Fx"E?#j>hoy])!Y(`0BrH>I>="4VGQdwblg%DHJ1Q04wNn&T8gb##Olv=!:j#;Ph.,V*pg-v(djpRE}mhnccs3NKGus(8i_[fMuA;%/cke3D~U#Uc
?iM6}-/<K.T&9K|()q/:FU>>Zch:U%{Ou4.p:1#GqoCN<?_Y27jP=d~5Pt=UD3Z$@l15]@!X.AA[6.XSf=>h}M&+qsj6-I-&x`gm[J,w&LRK2(t-03^Jc%9EP#@2PB)^K[$1RVM$hV^jia%
W2oc9`ponD<uaVB^BEm!32KxyA;m1c|gT_@sR1ZNCMDuIRw`O+t({q*[s*BRqj6]l7wp7Z-mH."sHdr@Cv%)2
U^;5G(QK[l1;rQ;3prxBpmnGRxCPh_LXkA"HA>D4XbFou]JbU>Ad0,TV"t?n[ld!R?`nkmVNXqjQ?6u7mhTaYRovT0%H1a&piyLY"3in7p(>s?BI@xY;:;w)5vuWMV(ybB|.:ogE_oINt&jG^9&MD*_5<ZvWJC&n6<FIkt?#{Xf^1Q
Q121n5H)*$<ueT4>5mP^v^aNTf!=$eYG>t7<246>E4a*M|vbW.E5NAhoABd1+u(Yi~`d%bIOVr0iB.c{"TL#G1hS$tTi*/Mt2BtMPU4/5CLqJ)N!?lmWNmRH#VJ>r[5Ny[19T$D1/{&o1H)6:o+]DWQm)`6t:&>%R(r@;~do6P8>>#?aRr!|@]1v"yQYfPNR*4u~QnIF@<0<g)"zJ-lFDM6=CyMRs3I/W1Q@$t4##c%r:K6AX4J~XPHwm]N"?q*o5cd)Mo.`Iz&yDf8GRl[!(Fg=UC"2Ma5EE%[Nsz_M3[38C9QOxOfQ]4dNYb!u+ZBcr[2zL;YI0-&&:9KmbEEmcU5eH)"CLx+@[Z1Qq@p:7KbeY|Gw/FLZYs.-L?S79dGStM@o
h^-ibZY,KD(>]7L2Y-+*&wkE1(pqB-g)2po6opGweN~j3K*G(ljRAD:VcuK`l;VN9DBQ2j}-|=Tk#W8cR6~;!Lx)semk{Y~;p2Q(EIYk@hW(hGBtbs:dR3-O]8$IAeTt4km
8N,A[!E]!R{qW8KZm3~X&s`<:b&06a#>gTYrf?Tp=]MT07B2xKl0[kr74.7a_J^(YnYB`E%a+s$?Fo/L51v7zf{/3@Y$}8X,MC`0_soZ.fsB1>[WO
2G/O1/_FW#)U=tF,AHphEopeJido};H%0[F@FV.GVVR#4*}d{QG%T9WwHbFSh6,J70kTe%x#O7R
2X|QW_:Y4F9H6m8W*ap=OXm)4K#Jy&0/Fe!hAe}CYE!+cnVTX#_u+!Ze<ICwaS~Pw<kBjO5Y)O;RVF}@b"PEJvMVl$Q7.:~i<o/.}$LGnvRWrP+AegPW42}?]?)ILf/dT
b]s#"+%ET:m%&Sx%U&Q<v"h-sgb8Ho486;g1^;/b9Y@9AW&<PQ#_K;on!lCCy/UE,$U5W%{2gN)ixG&F01,`B=uWtN!8`&*C5h"KLStux4Y,Ipws8KF<hJ!&~[_]cV-gN;[p-yZ/S6n?Z@vh(Kr,W(b<v`fy-1Cgy!fCBkW8(l=x).`6B+!os@rXPZLF,#QbzqAX`O`Hm->TZM@Dqfjs#HlI|3)OBU5TWXJQKjO2+wXM`6k?`wB4AB_Y3@DU@OUP^@Cg.ET<uW)-H2b;W$|R$!cH0c+"KLTr,Eh1!?#5Q2iQ8_z`CsvvYi&V>j8vn
3?KF,Te*ZE}4Z()^8RVB+AjQ<6``p;go<?aat=woO<]X~-w(.<PPR>G,U85m>d7u;W"9yX)=yQVY%^i?NRD%zZo?lmtg~U],t1lK+vZ/GEo,v-[:r%fswl=>["R3]qQ[%PDNlt#J>uJ0"F}KoQ6f$-;Ms*P.Nbnl``,j+-D%qr%T(lV&Pe>;<EN`:"@A]>FcSP!iSHU:Gqx:6f
eAdKeZ(].@jbxk7AttPz#=Zaee4wZZ
d!MTCo}Wb5|kbTAX##@X,hyx}HXB!wj<O8F-Y^u*qr,RI;/#p^ZWNPl0C?XA{Z%Z`PhK
/@3<x(]iCGF^??
V6,
n&s6;(i0=CmU-:^Wm4]2~nG,g+[Ql9q%T#AuL%.mtSTqCHR/lYsftBWEN-qm#k,#32B1#XsSI
cXdOeQc3YD:oosr0?Y-X+tP&j5sCnGJ2p
8M9:m^r02QgC>05w!Kwl_bwpA@iX<433
3Gn$S%B)<3UO3nZ}4<T6TH4}I>`KWyBj26GRe`m}<@uSB~_@9>wbTLfF=k!i!)ZrS%D%dY-_2#e^p_jjOSgEqPo9E<:r8k4JIEH0,WN&)V^+X~=umV=VH(.XiHAjb9fV7ceAi!U5i.Dwc
fb1rMo#
&u5`qS2T0T:V5)*r:7vAc)@V(3p`;}V2e]sKNW2r4ACc`tN(mvKF+Y[uZF1_Z`C^d`DA#p.XQM9_;PV<D9eERYt7A!pfB(=1.T[-
*7y
IWa&.wFh<cP_}X4#V?(WJ&n&b`o.EC$yjujbg0(Ll
6T+6f0)x"E;fQee</QfFr
9P~3pe1ZjNzIf%3r5G%ZMkZX>ySh;*1M+&Em<HcOE1B"SjN>eZ?%K7S7:v-Wt&x;+a2w5gRt)/AlH9Kp8?[(p
$
c"><o*DCf1UHMr4q7I8_2d)]A8~0`ny1Oh<1C/W>xbR;A#6bD9exFeH0l*<PiDMQf0uoNLwjy_,xn@1,[pE5R]mC%dnSTw3BCgt7,^k<X+>`.[?;NT)$jrZ$79~-UIGm!_jyZ^vPX7$uTMU2
E@l?CVRO0$AVnL*Dr_ph!$SiZof;-(/r=$<Qg3jYddR~p3ZE=~E-C"a"QX]5]e%etFtt`_x)DYFnr%Z_)"3&[iGT
z/BT{4;v|<U74S[6n5f7Vwc#pmAF.k+(P(Bj;K[H|0MqiU_QH)y/0.co{lvd`fG>Ede)[P%
5+biN9r!_2MI]eA"&y+cWh)
cg3VIBCp`W~(G0g6PCgy0^Bop;ckT91dqtFHunGaP@tWkU8T}-yH@eDwP+14
)46t0c]wJ1x%o@gmXoljM$FsaLA3rG
q,;0C9u8MP.cyE-8
3=j31a=KE*tdP:DJb?-|d+phL_WqcK")[xyCo->(GE<GbEaUycFj62wM#*]*D]%q9i35Yg$b&cK"CvMwoBQKv7->tMV.%bdj-zI>!"8T6,C0iv3"M`Iy^
ovR#f8gr_p7v9lOs.NJ`a!=fosw`kEpL)S,kemA}=NCC/3#oxL
Iq.D)UhdZ-;=>)|+=50Q}O(2$Wbjb#w42%U&FEl5d<YO6F7i;h~4M^uJ0$;/Z
X4H5WqpR$H4=|Z><}k=oK?R??WQIEo^
rp;O]%#a6637uP**kAru|;28OG]yA$>EFZ_9Jo_a$u}U%6N;L+E7<#iko,](xj}
mNa1d3j4
ygw>H6#91b<{CmcKO
%d/i@;SUA[I=*~-:W41?`
?`*6#g1tg>/YOqH}O[dG+X3&XBA>9+iSX10#U@7o@(?@=Z>:WX?2Pr]3`dEg*Vqlmzf)T~8Y1Y)%2-&m3M!l7}lT9^SC<qb#QtIsE>qE>nUC^vBSq2+m6emfT|we[cx*4l&<!r;E%uMlX$]q=w;?On=)N_N?SanhGB.
Qg+TVE8"cgsU;(H/)da*Q6(GKmgTc.cf!X7B.s&*aiZ*YH;3Jw!ZSI!-dt4?7@-%G&"tcfFNhOPYv#-.;9+3?[4VNiiu
BRy,/=O!V>pU3]W-eoINHQ{gDrbNjyNfeCv.oXpB$gOiV+fL"F%355JY_!8tt]]WqXMCUgyqaEx=Ko13i`QUIMY,:F^b]>E1&)#FH.XbAF8xgoDwoBlU6#%Lt>+A3/P?P?ycHRy]DcWAH8hy`t&
j$FStmxDsgF>LcsBh)JCd.p<G3W.#o
,<5s>4F&.mPnAk2[s^7%dB#d+)!a&T7`:[B/g;9U6heQn./}*^qF%$)}p2_D0+tew(Ncr"_gW:*=OW[Q`1v/A*b7J4)7/eET)NN3.`cUB^Sm.:(q0L@PZJ]8gf+
0(QvI<DeC-f=ea:SiKyS_]ew+}8wlNF&k"gxi?Ox2B[R6H=X?Pfb:;?v#k4}pfg:>LfhPHD}E_!M<p/yGz;wc.MHj=%`Lbf6IYGfB3=eTA`4hgl
[=Xe%=hwHhrjB[W
nhTV;r>7U1cQNSk<9YuD65K!"ZSmg,wR!A_NE6F!"A
n*f.n:}vA=,3UWs6=,RI%cq0v"Pe!"*_"J4Q]f}0?6~TFA;)Ul
LG
,X:xFVP^JU^<2*+`d!Sxc>p<%)GuPTFE*lx!c&@J<
s1L.m?{
h;"?;42+Ek-3I/kW>]Xpm&{U(IrI$Jz^Y`>bCp%eO/#-k+COv_UOa0c+@)PoXFVg}s@O]Uc5V(&a;YG;Gi8t_XZ4RcVd&PQCrlok3**odcr@H*o.En6&]b;+<3o3iV3?:ib=pX8anAfa:LLQ4Xn3Ag~Mun9yw?g>OUm@&0_6W>fal0DG)_iYGt/F+4C;VU:=EnJJTd<5*]2t_UE1`uOZ;.BnQB5h`V4Xj+nMANkNWB%U,L;xI"b_CY0<i=fDHOaw>9W:Pw=<qUXC|L,A6V&QdWMGd^pG4vVUX)@D|,ye.<d]Cey@V^h8eqVoi]^>SAJ-!=-l(<5"NaRg/auemy%/(<.yE,>)^14g)n2t8?xa1__)`jFPemd!~;yfZ`v5mUOE./~tk])7"pC`,"w8V/TygC%';break;case'icons-70163a2695280bf75edba563e7b5471b__273d3fbc.svg':$f='.n1F;boZc.Bo(o})e?Uah,r_6,]#*f&vFTX.Are>cU5dz3=O.b=K:xCw9_>Q|]_*[eKZ"sSUICkDTRCT*BU7|p[d&s"W9Mp[,MrS6yxy5+)_Hrru:n-yzfmIccln]H#h{29y}twyT!RpdN%uauzXwpktSn}v[A>wvv/rOyeKy3VK=t~F=7gchMjiTG0nqFU@:^Q]q7nujs
w!w0k~gz^$oXd!mBS$w?wMhn,b.EtK</c`stLO?duuLK$dxKSPK[XQ%l>}q-L[fA"ZhiZ3t3`g3VVh,D5Uh)$R+zM,l66LX34ul|[)3_<*X:m:n?Byj^pwGn?&H0GyxN6#upbP825@AIDBVtmNL}8/jQ<$?W6b+^1=Y49dD<fRakGoENtQ
?F`pI9_ITA#VMP|a*`ER1bDT{HJx[Wny}N_cj]SZpX!R1jFNa[kSlh*d2fic@bRrKbQP/wD"TiCL+?uiTND7`+5&:c&^5``=B5h9DLTuJAP&4I3mR8oG`R1H<ChCF4xry1T+YvV_!iB`|a=s@$c)cEjW<EO_72Ec-h/kmp0wBm^ejFtIL$wYM2u8Wo%"#Vcwz,7LH=.#
HRuwMRSBxAO]He_-@NgIA&Qxce1@3jUg_hQ*g2
mr}DDF6HGv:;s#D(B@v=LD[5E;Y"klP<|+&]Vx}y<Q.C;]ay~VX].]9(m^4hx9"2%l0WP8Ft?%
7L]!1wu6>{6LK,iW>kDaTn^JJ;"WU:/u#;whFLC`?S**Ex-:H`T?-Eh)(elz]6Y&qt[iBic`JubWv[MBWbX28fRXw`<2,2J5mPARa(9x:16W<"EbD^<qK%PO3CCfH(*WI&cs4:b1-Co8K)(h^7a]=+wCf05d>*?,9nIt*EE?R(M^w1q+ch@<@`#Y<G2c.72_v"c[]`$R6m<!8Z"`>bbj(s/NJQ,)eiD-XP1uQ+5Q
~Y
@7[2>V>k"SBI!~vAeC[)Esr--4U3QQF}#5RNaAgRr:Rz4~;})RKjJ%+?*]NT16D"REy1i$2t!L83G?ny5`K76IdA"@[uCh>&aQ1Sm-Xp9Qe}I(4f5y[ZC&DtN5foOV4^!EXnxPD4:L6tD:#M^ow3FX^V5XZ6h=A6I4U/Q_c0q{%x.VrB49K4
N=JPAWnuvSXr"Y,*peSMxVhV%TsBeST
g6A/Y6~,nYh
!XXvbr*)^6og;Ea]2s.cRcj]qyF6sTsP&^*7Ms@YBJD+x&EYsKG00dQyp4X5SLCSlL=U>xR#<yNrB_
dsod)ja*c=[F><yhoYYLaXW#3u]yiWS=5?:roMdg8>@e".pM;>9yQ8!SCa/OTNoawBMhF.$z*BMN"N!BgA#
1df]o`.:Y;pPvlv&*zq|&
6CY3I$4+vId,Ft3UVzJ.U3OcS/X&%/Q!Nild9}4L!c!l(d?j,&e6qFQ3P`cO#
H5u[G&k)kQ/g,)+q^BNfi`Hz!N?
e]^Y.6"`C(tP-f%H#vY$"LM
5^sTW7_yR!o1h.43?mrNhO8>E^Am>{d-%
28NW!N>#=AZ>&aL35&J#EtJ+w[]Mw>ux
JJOwu0HD5-=[1"f;$_~0h0($r?hOIe*v6S7N?K]ubK)[NTa^,03.+TKG~,u(LU}qTn-v[nMeJmNo+HCw/x%4$%ARi)O[n68Y{BI%m/%.IAU`#/N+z7H^GyCVSgD+=[&KMxI3g2I@?Gd3E=>9W@ctYM#w>y.[e`[MNqeNG1x3=tK2jI^R0&DK7"-PsYN(pVrK[ly"n:+_+5F)Q^N<F,0-kIMW&#2/1r"NEbmua
:I">1H7%-(~G|<#Q(kpyBKL7~$M`*,K:La=qU7OcR>
wv479BRh#P_s%|ns6G-o^k`rmc"_G>yJe)2k.84)Z=4QSCaO7oW50)v}S{wq(93bS:,TL"%<6^Z2.*K}0AeNRQCVCM"eJ:S)$+^;xd+pLXWmv~*<9Y?"kC+BMswK,TJ(-4<P</O[+
D/Z0^MGMmcgt=JeRg8cq_J=DC-
L+}P!GxtNwmm9B?`yBn+(&A!:<jp(5Z+}h2*1iLHYw9AV!.OFQtVf>$d%c(B)hibE:gC#jTM~Qxc2hnFh+
kZ_OT~ypxQuOW=x-q?jw!L@-/bFd1*`3ngJd)fd#]t)vl^^E*yjXlE=p`TRpK*^`UQLV^,3l!AYf6GXuyGybRJg^LPA.ml<C=I=WK4nR9;V%r@&|!kKtcOD#OJ!8h(w0cdF|Mv`n"1QSB.graW<W6(w
l(rJmli/sZ`t-A@vi;G1wn1>Y)oy]K:1K_>@W5Ib.}DJ5&9SR^YGk1y{<j
FN6Y=yYl8[|_c)wiJ#*Wc1S0d"=aWo^B~YRIbi=!u(Dqy>U2t3yNM?m)::6<B)Dg9:T7)3fUT&SCAF91Q%fp1Y,wUP5Gk2#gCr,eq/Is6fp1%g&R2>WVQqmhC`]mU:q%>[N%SS5#<ri=X`xqj;#pM2pY+sv;~Py94[E^%`wL/em*},gg:RVBI;A::AP_MAXs.%aJM^}<5aqO$?$`c"}!ZeF"P7oh#n|jYdc/iNyuVYEY*.,"=p9]L/u
#e1<.
P,kFR91P,MB+(Tf:b%j2mqwT
@G4{i`n$pC[qlCGO$AOMR/96/d@]N$HD%U`g<#md"`3:/x^vS~M]vpow9[xh9.i&Ka$z3w=.v*Ymq3do8LKfZX,pelu~HJW+go4`GWv
QA_-k.L<9dHfc==u1T^V8M3+fAhjVyTGJzU-_]7*Vy5:3Q$;_DX~uFjB.jO?Ucg+V}:[P?I$8FF{p2/m!0LkeBGm29BAt&X|%V`zyun9qv65"*uzVPe%Qz"Uh)=EG=T20MFRqt
"bhn{AjGhfu5ZptI0^;)TlJm)&htT7!
odWO4S}JR)A3MT1;pN{#
L-UJg"8S^L
;SV5a(sn{a5t53nqg)HO?RHo7hxjKg9yotZ!Me]`.Lif6A&)7mL:7jYF:g;P~Yw.___-`KGa(j`A0EvPc:(p7/ke8R7YQTT%=1J]lRc=s*.U4Gf"v:FCXL,CA(M+#_Dm|@l"AU_0*rajk`%NfXdKQhp3Sp];`$c:9&c`3naA-N?`gTgkH=de9KEEo*GOGQ&V~2ST+_sfS%pdLJLSu@E:g
U:!4#&(%n=
u`OOeOPu=<%oeON+QK
Rhz,Ll-Nob^yvX[xW#zEuuXdkf?OrI@BuEw/2$RL05~WD$
oChHd*`0eR1k*"@5K/k8e="BC46l*{t%<r5:4GdW
S]j4m>,3=n[*-o~RNbd`c#"qB=rcYDmI}%V#j8dUBEkNLVvY6,!)/.(E{60RE[:%wg"+r[Jhd=dvXB%il/oCO3If`3{e?p|d`&fE{i{yA9|Kqs;J>v1MuBU3Eql1@,k
$1q/9,WkOL?nQkXL03(4Lx
_Co@?A)n;I,3m)9A0>n9lF)|*4*S,9et>sgFD|#"BpE:Ji1LUxXz#"D;5PSoK|"LMLbT#
vG<kE:cJy{%7n9BCg/V
vrt2E._at*pH]&?p`FokXr:$.M*Gyg,1
R7k9lTM*U^A,(fq7O[k;%?jRa[;36@a^w6-$Rw@wz#62/t#(f9qPS/u._gxUhTfP>,u2>ni?S?Yo~YkBYjnRMDhQa`g3v.q
U)GRR7!O`X?/u?C"]WIJK]J7p-"Fj>IOJgJ,h0WbnP&[l2]Ve8-G;ax.SRJyPSii-RL?Sl@Nws8SQwO#mrs^
Z83;elTEmy7".l($?F0@av)JYeh[Q-Dz6uRCG.GJ"8S4=/>%;HYCFG]TYjkc.aP$Zc*C3&/t.@[;Uk&Mko=C_V39slao*`kV_+a)"n@8C50V(VZ3;T.nlAKB[&j-`01)j8480s).){JsM28s[M%5imBcn##%51q7:m[7CFVX<<k_7R<9wtTm]?w4
=(R=c
HD^ExT|5&eN[FXusRW#Z/0MB`]DEFK2]]94(bLJ<N@P8g5ah.]hJNgXGDhaUo1f^eFeW?c@a2>iKH_$lkIw6}q]c(Y0k45JOr3%yar+v41U8&z"tOGbJR??&?7+[&pEwlhpb
vNu9WffDDB=Vu06L@#U_
r98urI%*u>cr:e$o#5aGt1JM1g]V6?zx
3zhPBi@_EJJL#Mt@&()Gc(sK:x"Banka49_&,Bi=h!m(K-k*=qkdI^#e8`Y12~H;Hd*75o
%-0E45)v;7RPCP/P.3GR2&4tzp`$C^MCCo&e8S6DkLMOfm*hG]I9P!52&2#Dkp=kSrt/s3aX{0@`qL&RKuSO2H_Ca*|juFHQrC&a~3IThG?=)#%lH^!?~FoH-mhAAI7djw9/3G$4TovbpePv8$nXr+hR,e4f1DlOoUuoKiP:|k=;5mO1
(._n#SgwU$_5WILBfXWBm5JC0ne*76[HcDY
)i&
.=^lz(O*kaVT9{B@P.hg
6GORrZT&PBL$(Kn(_IghPaGG@_6IEbp`gRy$HQ&MWS=b^rlqr
[>A3:quF5R.
pB8D/ckq[5-4B.:kaHew6AN4Q9U@)J]ef@_V:)j;K6-Dy]Pr1OFo`iY8iL{D_a%b@SuC=i?o
!NAM)2#TW$W%HIlnFPMat
a~P1Xc&Za1[e78sF%uVvx%1uZ:QU&B`hebqDV?uif"^_*`Q*&4QfF^gO)Tj)&,Sz6/nW+@hx.Q3D8l<|I&<C*0#26o/zdn],G*+.J9FEgB:wElr=wH/F,59FrJJ0yadu6|g$isS$jbGv_{2U0>OGXkstPAAy_DtzfhXsa/7dk^!yU}U4I`(OWvW`MziHrGscTFDCH$W9[r,iUV9.PS*
=f!/$p9-1Q,3=B9!xOAHrOH"i:ZHHf<_YjoYCqy.!H5p<E#w6kEUS9,?K7GoUH-=*}$R);Fy;ToJ[3O|S=Yjg)W@q;^:;u:DnbZF!>,HxtTTqsnt!Exct;vXo(yS
7V<$jP{XQJW8{ar3A,EL.kdM$GnWTdlfTjJ_7K~it.x^_$vd$It&7k/#]q9&%
I01n`
c5<YFe%I3-U5_<f5xrV^`ji7lxs_ya47rth"{C|v]7zn4rI;9AxjmK+PBDfmc+dyvV/wO+)=KT]
KQ}gXNCjek"rl_D_KT_wPqL!64S/u%oy&tKz#D7o
o(I<bd3TKp[,cl4F@KnAA#i>KA8Ij%.=usBSqnm$tm&Py(T8Zql~K2JPB[.D],qGLbwY>TXce4XACj0?n8?"h;_F%(.+XZcQU@%_(.i+>r=}IyFG!wG-_F-Wg2dTc69Vj^;O,N>vHK4-p%Jp<Xk+8O"?vLUxW,6pRI`@%;Vf3Qgp-xAhyXF5@rPXW,(@/6E=@`>DqIQ
`l_S&TX(p5OxW
8
Fi^8.OyN_m-`nFPEaYhcg[,N94N/>/0,k!tK+k<Pm(pl$&>B<8%<C9o36y>1-A9gx7>&mUO,0/lM1)`]7g1TZp&H#Vr|]m
7//$4lxZ1*j_BI[-P-Vk8F!mv=[%kH=.~e)57(-GSGK;DJvd)A)T_:
y=ke/(ag8*;C9x&@N;+hrV0Eh&D15#3Tg4VIp/9
efKZ01/ZV6_SyJC+-<2-ugD{pJ"%4qKrorfc1q6fadN1jRAM={%}6LJJtT.6.d9q5#K(Y4o5rS0v(k&B
Lj`&V3]5P,~.f.DU$e`sV=@*@J2]l6lHfA>2&M}9[)nR"D")#O{uMwv3N`K]$3ui:$38HpKRu/=jy&>ZW%{m^c)wl;cEOh#($:kt@0}wcJ4g!k(@m"!QN&<?rX5.rC5Wyw&"~P0I3CEH~4|x=;wy5>1mlJHyhMrm
XCcst!y#ykyEA,xeN&';break;case'default-blue-e7acfdb81453b86f081569afa115859e__b628fd70.css':$f='*erWObOZ31.Ov9U"l9Hk1R@SJ5kdLhze/CTC?saq5qsKQ*LB$<b.0;*%{a46%qE6[mLxHcqkHpxcO3?I!QoKJQhn1](JH^)Jg]@_dx@4B
]+KvJa,@FjPGgn~RxrfyFQInxb7=N"
ca_Ps,V/5(_^BsV(x[HlmPhj"^G&<KsPxIy=heMn1~8R7rm]gn4TB<H14Bb
5*gUc2hSJh[O4Fn-m_emUCb`DS1B*iYxo<W^#r0YD%aKQHOa.gJoPg!XuP)v"jVkmKA!0(*xbYX&0)FDuarAkd8c@1_(W]9lKt@jMZrhSDa+_2uvMqG{Px.dV|LOHAv$P#7#Cf`OLr0&odpJQ;y3=cTS&bH@XXyg,6yBf;0R*N1.[B1S2=3avQ)6v#p15aVawuhc.8t7H7HB"OmP?0vHE7ZHS~ZOMi#eMLV3r.$<-ebY$v/GWq<x!kCT6By94+5EH-H5w5DgxnCNSW(?AIOwUz_PeF1z%eE
!9:O?r]$nTlv!L3{`K,BciM6uMssCTy*aZ^Inc?J<GM[sbXwWpt<l>yEX9a;h0G8k5Uz34xo@)s#t3xT=np)7zhoJ9pJ`GLVyEz&mja:l3r>JoMN+lcrtWwhu&cPWZ`HMrd#5.w8<jMrwXpJz!KmyEn(o!g}MRjp@[yb9<K[yrtO[jsO<Ocdyhu(YJx3_qt7[Nu6^{`[cp!GqN^U3470o$lsL?K
oHpkohy47Xz$a6w5spH17Zx.lIuMyV,fhlxKJgRl!@fi.DLpX}EzL{W2>-Y{12]1[oY#*ip
@f"AHIC=dL_sU1KKE"wMpQJrPxmeq+m?9EB?8NWs3(`79UZvD^AcwU^>C<_tg|XAEVC@+lW"vQo,"GNoPb"#N~)XBS,9L=-,dv)0.V=t[3"U(G0d6)ZEETbvOO)#$R&x#Iu8+`UK+=7S9"W7qnxBS%Xsqk:`^{OH[@o8K%G"WCxzPQa*Nj_^rs>0)fl%^e"`m2*tlEJs$xGqUR2nQB3$Bt=GN&><h=&hu#9Y:%XRvVRB[ud:sG#@[_gQ2Sd*vX2khv*c!v5rNFK^V0Fn0o;p3[Em*Mjv!^ty$Zx(Cck$OS/aV?b%m_UwsU&5tkl8I*sN0g4}^bL
n<M].!"VI:!l;>m&%l%IW,ybRJ]zhgG$[~M3wx^SNJ"x1J]f6,XIw-gk"y,3Ey74-6M;LqWj]I9K^CZr8Vsky
f
Dpc>JI=X=r?0C,!QAMCS8aa;=
?0*avYf^z$7QCtYGZ=J)O>qya#;<@Fv9%ZA5s_Fb18i:v-bYWW>0gss-jl`7n:o7dY`&@p!E&(KQ]QiP%<TE:L26]Y&K*?`Bf?n0D|Oj1
(UE7I%]SsE".P|uAv;obN<.XB#q</?-M,6FK-&>EMkh$Z$K=k`v2f/Dr6rA=D`]Vc"_Y
xj[FG:-e##aGKu~jS9aNm4X@f<Lk?SN2MIQ`[qJR+,5DfN6fR"7%e885-9T
w8M,LVZ#R%D$rAK1uk(BP=Uh#8P,3$;(epUbfYopUOyrKiAkNcr2!p%;J>
S3Z*Yt
EjN>Sa$i}%udPWT-x?Vl.foOsG@N*5x^_!UZpo)Ok-k:PWga-UG+iRqlSQ:K09Q3:O@H}hLDV`t&s)ADv&{S^<Yf>wlcZdr7;o2@/^%U=CX5&2YY,Jx;q/?=s?VF
#Tk#53!~V(k`ob*&`vc~>njxN@l<*sl-L+cqAM"C0k9L/eHwr-/C9hQ?v:*T,/viTH.-fW6UO2yAre?bp6vnwrHWqY`DtU>m6Jn<iDJ,
G!>[eyf4
_YP:L|OR[L!-$@ncuF6[MywcQq#^[rjlMALtsFvr8v->oQ=JD^4ux4H,5!(_`HI|o!FGu6;!hG4I_:5e,6b~NZZ=`DMIn`2#M
O+Rxx"3EvN^8#95<@uSAq01>s%o&a#Z:.T$2e]fz]-sM((pic<_/`wv-J+.GJkn<T[),Vx<l6BU}.2QiwlW:HVVS&}8?HWr%.*htSo_8!w5`y_`$%)QR8-(5Eol@n9t6v^#0
w(EHq`#?
%H]OT!oS]V#=ACtNg}0Ni~pUOfJT5w9~No_eqC]g+Md#gD_,MD6XMDreq}xVr*ZIm)(NUc
3e&Y7W7gv:[S}r;*28u%?)>5r8OK|S9b[DW9wGI2yCW
]$r&jt;L"4%"(c=DBDHuFW2R5CU=eV^:`u7F0,Ir*M)EMq}Z/%C0)T*C)JJhz*#5vumFuxu/2q#+6FJHuOh
x:cft[}"BpST%w~-b[AuP5%tAGt3(k$%|T*5*yUvRHT%^TH"MdMo4;{IFlC@Afv:)Nm-:s[(c6ijayDCMu
(U)C=)@!-r?`1>
m8o6NXttuR^Nn.gtm[vt0miF`eyfBuA7htI_D3#PoEC60LV#Uro
RPN/!,9V(bGZ?C}?F_/S*U5:)-OkCQAd52x!c?M[q!jv)W&<0,J#Efr&wC|%iLp@&?2i6
/1Ulu&zk^Oxa$8<Tjfdp{qkb4nIYNF@LJ>.P0uEHkS<.+6Fh?O]qzKa8E!~
3*TWk#)F5d+wFhT]Lrk4OD?/x,Q0~D9@Iv,vaA5rS8]J8HUQ*$?Dk(%%h&-NrR6f},6eKiPd5fDy=d$&5yj3o*|]:t_ZEv{>ZmT3[w~wkTwW]>D;i^/g:^-z(-Ah04=wsd
2_Wc`),[<OhlF)_a?wlJ+PLiTt%<;2CN-6!?,|cqw%I1^V_<j6E/=`cjkJVWmS$L>c.Sr+>?c!Cl3@Rd<71|UIj96J!4N{d&
+j<DL8B>p/.;Vrdp7&gE~euZVUs.@ZvndWMfg(:vPn,SE%ZUk5mcqAISjm5J)g[fVtv&a_~&q5!AaK2r-B6(bQLkqRP_M1au1p>$}(ZlwAD*~T$8,
:>{,IoBeBNYXp>p"x@tc4+NL!#}P8+ONUnrQedb3/*9pN3tS*j4$Lc>PPi1pR8
g%i[(B25:<5{J;r&K%$@N:[b#VAoY~*QN}:5SNJ?0}9fGq[>JG4ic~es^g2q%vbWj28G
GKqoUYt#H51Q6%{!Wk[Qo?B(pk4VB<O05q
0Wkj18TIGX<sBid]9(we9LXC@&EJg_NJNGhgYHw.U5L!_^yE@L!Y))];Cr7~%,/LVyY0jcv?3F&afmy+IUYH%+,tXz#X%Iy8I;w{72It@G$w6E@U&yqOqK!0ot5w9],x<)howY8JN%Od8Iv]PPIsyHb/G8OtGl-xOhVL)*P0q?Q!Kk8xI}XQdnb1chLsJ)w2P3hj.%0wH*]#&cA@er-hbGiYrirWGlI8AO*Mlk643$Pd8UpW&_.DUhg$n}ctyhpF)a$s;`Xf<ttuT`mdid3Iru509V((
y.*dC(4tqACD:QiT-[rZfHY)`tgvE)/wS>N+g
59XPTW00byHGL?]iAJ2n2R(tasHn1
,Yif^`FINN{@tN`c|k{42Q;8^8{
3gXYKNTTKO0P,,B&srih42a?vd*=Q6W;y6sikgW(*em(a$&_nTtEcgk:G,)yg.jr!s^>&b_X~te&("1$R
a3M6`x27P$s:k[qJtQpoMc2;%FndiZ.wQI3quiu`bc7suuCVY[rS[h*?VlOax/N`=+f<B<(8PgFs.<}<d(equqAYwT=,BVuc/vSGdT+
*ovrE<oTb/iE|Mtx2s}2j
7I0,7e:9+>DR7QM&00}p>&s+~yjy7%!Lhp5./U8]B@kn(GQ(x2aX"*wP55SwIm_YiLjan+t57@Kt>6*SsE]
Ug+$sW3U
RO`GWn//jr+gU.X4PQ[#NeX:E,m!HzP>Sh$?Z{ZekKe#=[;m,sY)%-BP`^(SH`x"ttr%QB//p&2HG4u?kD>Ot^W9yy*hBGH|0e??ts$xI^;kOCp{`6/4aR#kjkmt0Wp1pP]H
FE3V!+{/3(&Aj=:hyqh5akm>-qc6H"kh
ZmOcu%ZK`G.p.l*#=$=a>vBQ]r+`]0,7t/]A[U]?Nd&]g?tZ"~lqI0x=6r=V"3HmWpP}_u>g+OlFj{v.8#L~U](=S@Wj,MqK]^
O=<%G/yp99LlQN^1D?;`-Du1drLq&aW.DL9BlME*OT?P.Q~,o0(&|wmFP-U@^o31N<5LUiq-I1r$"%?xl(@Z_iL7gU^%_-ek_>xNB*V"u/0UocPm|_.#cISO28;IgEB-(fs$^V+9fRZ1rSID6Y*`&xoDg*~-kc$Y<6SL3l?6mVU=[oD1nM"=f,U3XdbxvaPF@GaNVx#$4u~b`JK#U9.wn0&(Su!DAiNHz1;.W)!ZtpnoK^WuJr"0}4$6)eUx6r7Zc;VM<.cF_cJ-4WWKbH)5axE/O^BgI_ke|K!+b^.d[kM-!Tv>FN)yk`YN1"JZvXE--J<-+j.=}`
7]-u6""0&uHP!rlj
D+t(OHtPf-:tbSM*hq1HV-&h)`F)~LE#7f<v+5T^yy5Mm"#&:9
w14LJUoROc"/:Ps1-UwBgc)s]t<rJx.^53a$c3C851PB*<hILW(*2|FD(x$.#}[V*Z(#?fa#mk8[Wa"5ZYC^O;81I7lRvVas*PRu<Z*=Z@3Qpi,:t!4*.^GI:b)_,:#uS*4(KtLIK?sP<62YMLXA`p+/HO)5h)Ss8n+?9$MA2kVvQ&ky=:=;02V58UsH
9).<:Y29UV;>%6ag(fd`8*$!)>xDil1r+gE-4GesG#][@*0(`%.Z0<y[k3^<_f=b<2H9hhN,JQvL>V?)JE5j4`Yg6C_v<iD,c;ESap=NR@u<wsVC](,&|SzXE=F<$])_`%)@kIZ-:vaj>
xah?
jrA<.NHEU"&D,
^U%LY~Gs
mW1s~w!l9)(&6bUgs!3Z;>tyArXo:aE)*TQ3d!34:QYc"AtkXUC>{a6K#q}o@t2&?t<c*7>_X["@O_HRg-=X^uC
k.;Kbeer4A{@wy4P/H5F<pQ"6#c6|J
.9Fo#hA_0qIO6xu0Zpw7^Fq_t7VDa,7c/dH6Om:nJgZ;rSuYnG/|<pyN=qa70Z2RCZBfZSV:"K1TOqd%UBUV>TI28[OC+K(^dIVs/wi)!E65Y*FV?R+(
C3isF"%Nm,e[eA&0F>]UMTPZw0WR3!c+Fa&3^r
`5^:Y[r+j19gC0Fvmnp]4!E[sR3JNv%nC:BrK,_v@?3kh%gqe36poRe{R9(so8AgyPq3%qn4CpnvH)X^/[OgQQ(N.z-MS^smw;U<:G7g%KWByu,{@P8*HE(ZRCk:%<R"E5a(Nje^14BWa0Q`=0d3>B/%F_3H7-&!l^Wxa3>:Os>$2wNqhDwe`(x++_Yf3V2oqM"rp@_Nc7v^4w(F]UL7"0f"hiZH2<qi9?14"}UaAHOk1DT;
xR)sYn:fJttGrN_S`v_`fhVLHTj!s:1_ud`K4[nB+,(QSNR;JZ!%9;V-{Pw:tfV"i$QFAHdw3_s&eK0wM9]w<8i%RNUt}-noj-k^VTc0p6,C,g)cc^KG[<oYA+GjlCY00OvVXi6Y[Jud;g*VpZPAYh9WKx_8-%v)2txt$Wn`?Z6ve2OK&&6*&Vk$:WBhqnx[VM+pKB%42?iT/hcgGa
.DFhb:;ZaoLsPZ3/f.oh2JIprosS?JD/_+iEt?ne2@4Ab>V^Hs`pR0MTA0MMu!p{Zfg_KWR{BSLuAU[J288L(<q`,HQ_m..Vt;!}0PC3qSOD&WnPc-->5UTl)S.}+oa}`%/eTeBRH:Oz:c_ecH`}<C8tePqyap&pv>=ad?u=j_()8}AaMGml%r:w0.:[RC8X>_0CFSWNqHpJs0J?B01JEeWTK5p<TS8tX>eS2_(W5&NN^
!XIlky/[/
IBKVx}m@.l+6U|Ay"mu>_&>2sgxUe?hd[JCU`K>`1.w&_)::wixK7EX3>7f^Ol#@>YNFB69FNX.%hn+;2QxahESW*%=g9PCwBy`T=3?u_jgBqg;?ve>8nD3Dao8yNjnlY`D<lHcivi#o!4TAKP#3U=*@_{MIg9M
r{VJ47i^LOV~7`K@cxWt7<:%&dvbrUpYZ[qm4VaO1rT4X-Mdxb-9)/U3<o%_or+..xB;urgA-W0nHV:Kq~?lV[o.]z!cc@WvL7bVM3+s
I)[*.0QD{Z(OG#OUPo4utiz?
B>Po)xmYZ)r4&+
4h?^PqQVm[=#NH;`l$fw1Pd_ufNg:MvRY&}&1w:HnIwRkqC@2<%<c9!8uFhMMk(Z:/>FKnU)=:)vE27R#5ejtRPKeg,."eBYeZ|hp
i_QVfEsKg48n@-mxF!EuejoAZC,nrF%fJ*:Co%*lL45(3,RQ0/aAwWKo=@%-l]k9=H$^<DSR]Yr).s"6b^$O@saT,[5_h$tGq>677;_V"eJ&;8(#Nwbari$H!Q2EsVxJFMCu0GQ1qg(8Oqq!8;"[JqDEGFX9_vHaOr!B;/HYK5uEo["dD4*(ZU*%vz!hn<>gUwPt<V<"VkM@n=Vf%r~O_p"G3hq?Zh@+:tHgp(&`7gw[JGwBG,:
%VY&O%|JdR)Rh.dSQsnLt[:shf
K=+:
9!`@%
ptET!=LFbF~k:!e!`O&<Z"xB"T:`Gg$f~y2E;02e!6c@2wwQPo++u*}#0[9w~8~8)?Sd<=Hh|Zv/")!V~):gUu>M:Y|)auSuD
_H:v~O>2sGqQ/%8FQ.g-9[r-_*@_N+pP!U6$xi:`[Ql#QY"xJ#%fLjBKJ%HPq]53-G?0,^6lg+u"V:a:3]5r0<V@|#N:SN(3C44POCTt0,Mb/&go[+y@>DWcfo"_A7^Ec^c$ZWU]~GXwM^4rb*)N8#}SeBepF"^Jntu.A+O:Nyc$:WQOW0IW6kl`#";e}f%Wpg~E5m"bojgBR(.LU1XW2b-6vcn*YyGRnylI}VAS_q@v/(4MjwS]O#F]2+CC#na(QMxq,j&/ds+UlmY_lIl^@.Q11fqknVf4DODyN:hGya-x&AL#sLo#]C{W3.)P?dloQ4|[t?.w{c564vC#)D/1E5;hr2.t8J=J
h,A=s9(rMH@*T5@E)]D>vNCZ4_tfd
@xB1!3G
vN+zo?a0NH/H^hXFc^4o/XR;8-*-xVrQ24jSy;bh]xTzoq-Sl=a3RRgYM5[]8K8YYc73DI<NI"FBC/uckax:k(%bY],25y02OIwr$qG
PhxpVVyWuK0@C"ZX.a&#R:xUKRQqsKXRj}m7Mh(ti@LXsr8rNoVwtY
v)t__oD)H"oxlb(+x(N5#[alNC+=c2{4"9[Xd+aB;,e1Mc{y,[UXn&_WAjLp`[R;QvreGnwlxS%T=RF@30yC|hUl
LM!t+;-2vZ]BKQl.T5C4ruE-*E7Wro;}_jh3aFr<L$20*RKN*.bs;vD^RE]3sy(nZ!tQh&yoR1U@L}KX9I/H[MP}*LM=69yE:f_%w
xARQqHV-R]iXddjvU$RG")8jb{Lh@QZ:gnZc$9FhJrJV"|"
LHl}E9KqyhE?rI&CdE7RwA';break;case'default-blue-dark-1061ad7d216f143e3626560b92b66061__21b065eb.css':$f='*O{SB7nV?&/MUNH!{Cu5Lel#Y-XFC?m<DF
RA]rf
m-Aq@#Sc
+tO63[H[Y>"Q%z!L|tGDN+euwCBnO#LKF/^[v%VhqUG_c;;mTi=
e4C5Ui
[:5Sp4l@eFRj+{:@OU%nw1)P28n/04Uy%{O`6UPG<!x
?Z+<62_W""*&v%OIm{[//uNS0.@+q{>Q;nP
T:w39O*i9=y$g`.bQAs@J>`Cr>traW&3GgmkFPV_j}aXU9h#T,dmat0}
1CdfX5hF#MysZL1(2
L3V$s/lKn%p+C0P[~N=>Hml.i-?);DKa-wJ8&/7Ch!(YO=FT!79`boWZJBnmoZvI7y!y9@Spz2XX~bR!eu7FvZTqF]>QMdkQEa*7WYCmZou/AD_rz`Zg)+;JgDVcBCfy>f#B$GPPHiZet^W7l
!iII"Eg^ZW]I*Sk=9IZ
N+.wGSX
yU!jR<EO>X-TDg]VpE]V6a~C,Xe$oda,TxrMG,y^ty(4Gv`VZ
ZgEuSDc.|;OJJa=<G5)t,aebpZ;,o`/?9:sxtG?njup)Aaw$<<@sOIQ`Q^IJ)^f0SeBD|i{

)##tB
=vF>/_c%8tJ;2zK=ZwT30v
&mM]`9A7D=eQPVUqNVh=d?h57WL%rZZ$>eRg^P>-z=GTfb3kDYviv%STNwA8@-qlx]XdHL3@ipAKQ#lyUIE%w$M?<%d-sSG!u[{
Vs1vC&7.J5O60BJ/S@y&zh<
1Jv;3J30W4ViPPg53r@RK-*Q%$HjZX/c?lFSiD.0r/iT[)%G*JU)Mh.%L:zc<0Pw1IU
s?[:Ep)rpcG^+CWaw-,2&0y-MkHQ#!:HNMp+q=GCO:UL:z"+P]23~)F!.KT@<L.z%)^_SQXy<>/W]]_cu5XiVEfr>f3ng3,IawA';break;case'main-c1727fb392fab162b6611efe18642a5a__e1a04d38.js':$f='!`K]`nsZ32#tW"r=6.v"dSd*pMac9DI_osFcDC}l>Aw<Ar%FVN*VN>VN0"{AQ7|w,kBq_YCVqMRPtcWS2B-bW?G^+B
BlmHB,h3V1lSR.cwub:)Xs20E>./v)gB^LAGs
f-7Ir_s3*53jIRUY.i6m2(7d+sC_vlr=]5;H@IDIvHY@GPE!2+=oPjl
hIJrg^K(a{&HHD-7Ztyluh@4S:<T>|(q-_/w-2yN_fSNDwfMFZWcuI*w&u=Bu
q69$F,tB:46!sP$JXE
/S!BLi@Y
E5#ihLwhQgX4cY@+-x:ZpiY+s
K<5.Y7j,_@??.y6wwPt3_tv%E#
5Btg]v;>Q*,uYh:>tQKi$fRyVYAB0uPDA2$b[A+kj72`R1a0r>#-_r)e]EK4t?kpam--JgD
&iMJYN$OLwEoIa>*d]03AwXII3-ZyCo*&VN5S3/I{xQD~j$ua?VNdf?Fmee]lP!d!g*=~:=-5:=vVNYv~C;=8@?H<v}cT31<2_};3iA:".&M2.~!IupXej[aM/#MyO"7JtQ1fNUrqA#JP4b;#x#$2c
gwM$LZqXTS)1
J;<In6.IA_KC""Q4d_Mlf[;&UFy>5%d8UYxkHTrPoO,2S7wBexrC$l2*C)"1Yib+rv.)EBg^(Bmt]$r!8ROA~e3Xo]LS:#LDEcFFM:fBxV:xrWHta:2E,DwTrQtAr?H.@L3.D):tq%GqS*24s")6vCsb$6j60q{Rp0`1CMjgl3,y[pn@c0_)aHJyP1J02Fr*-,KLoKny`6+AXW<]
;Xq0b:FdL}N{g<$211oSv;Iw(`0PO1#[7JR]FqE4>V(wB7c@Brmh[6iU>"vC:a9JU*I{yC7l.pT6c~@vxfGEdJt/I-fzUdC,<^#<q*vVF/Jus-qn,E1MZX^},}@u<
f:(-0%<_$HMdf_-]_xQmC-xf"aP4cxuf2MwSnQIsd7Fzn~MNe~W{a<.]EjyqL6)]4/(,nQ`Q>]y|IfMB"oCqY$B0EV6V8"2f)IPce?%*)mAst]6Rx"%
R>X]B|mjdLmz3(
e^{TCq
!!IwP.LS
S7oQe-|]MeA)[:;"qj]+vX=p!1|bXFLy.vI`&1MvMV*63w5ZZPJ&3/(G{8Wr~qO#w"sF7nhN~$oU+iV,4^8?gE3$R.FRWP|h[TrrQo`S[>
$hi4F[9~+j*e%")<e{llE?2Ve6PJhQ5lkO>4.hV;%oQ`v_,N%hs
CoCTBc^+[UuWpX
f7h@;.yJ+=o<fA3`WF`^`0M[8ead4Zi1rg+iQLk^SV58STQJLr!Bq
3j^TgN^xoy"=cKMMY59EBaVHQ&ylc">nv5v#V^geqg}U53`i1)b[;5S.P<kCAe<U;2JCl6pgNC8o>%C([+Qxn0J
|7b"m9EnY:wFR`D:jC_RsxC&t_:<K5j5&siFM3<KR_6Im7uGIxku}I76#K?NmW{A
?_nFZVX.WHXqGsj
LBl}c/#43(clt&1u<r(Q1SVmJDz(]5ni+$tI4I@eo
g+38@{[{e6r#*>K[+_O?UU]]t5af2uZS[D&XPy$v5";LVTF9l[([F`rORG*`CXUQb
X
p8e1=S/Q*150$XEUI}"C-I?f3W![fn7Uh=sUBJVT%"fJvdLc6/`|B#U
9z0{QF0HXotr0a8Cre=/=Vx:1,"f1|TG
)qiXMRM.wCO,EcU3Zo#Bk;h;cTM(1:b,Le*jeqI
vL`vv2JwLm/of*K8(B=;zo,85IPu(F}b7(>^^_
318p"Q>4^Shg@o=Y`^.LP<ZL.6KqG;7OC@EmIJS85himFu_z#5HG!>G~L;MvWARSrIIB.yWRMt+3"Ij]oTEa,+,g_GrN^nCSkJRgl6Jm/PFDxU",#C:E"["ZWaZfDJ:OZw2`Fohgd2Vv$t)h@3@%!BRG?#yl*8xbdEitvz)QA*.B/rO-qVQPkT;0bVdxoY5L+SWk=Z!B`bHtlqWQ/m`W/<C5UC(TitHC.|LQ%q]G(5gbcRP5a#ucb!5;A4b4BlrBGqEA!+xH"^(udvDWfq!{xJvH:L&%Zp["p{o^fPpUwJyOju7G%yR5S.gNvReLw=jtJ8&=T:oI0e4BUB!j97RjEK:?TL,Au$Mr<sW;psf&EU*<T)/=J_TqnUs(IIG-Vw*(A[0nmJug@vsuiw">xknz?}aZX.3oXv-,Z)Ji%6Dp^_+uQ,c*CgU<<E50R~DB`JW~`?%nSnEmd(DK=$"rr_eLn12ZbvVp<8Ruj}NWJ*tnfSje:J([&*%/@@UhSw</Qze>.x%@&Bs?10VQ=gQ#ef_U6pNeF9Mi+$03SsFOB4wxp=;5*a(*[Ml5Hm_}2M.;-o5sxRtX0)aRf~7XQmk)r8R+O14e3)A!T
ut>2;hQ9_hhWkPUG2vEJDU(l(#N<M^Y
i?-tKjGno(a7]jYis9,;H)ZF.]M+FlTtntBbM-mte(#^Cn6>p%Jr6nFMtsxFR&X_x(3P2
q3jSkqB$:zXM2/gd)}xY8(;`Y@3Hn,kG]Bl18u6BIxe;YdnxUS0(x+xwB.N7_6mkDlk3gMZ_atq=(U>i1#M$uy/"vFyF%A!GS^=*H^@lN(Z6^_i$%,VJDMZn$3$~%XP,#z8AqK8?-@sbc?<(a%qU18IwjUU7lxPRW2kMx
7K&;4_Rqjz#%k!WHdivR;]rVPPEU>2^BJ2mkC>qrlfN9$Hg{+}w3"|[Yp(80+l#8!qS+*|t?V(n
.ZLImxy%aJpX
+.xt,oZls)Y3s)1Zap6AK1#(q"lwc6P+K$rGZYK65bjiJcfXe(8Dl?s3X*97vs|JPrhIBVVO~#*
|Hiama8j"R&=myXQ,&b[&M],z33T~R_vXmN,VeeVq<9gD[xo*Una[&ZQqgMx+oJpmtL$>V]Ofhmt^Eiwps`qgXVR~YVo.U]fR.ZBglQ%&^?KJrPwJf[-f
ClQ=@XfC*E#>$e+kT&a-zc1N6"B
Kp.5`w3BR5]96Y,NEYDItviW{TBO]b7,%cLsQAC5cJE=7!TU&A8?+/!Wc
4IwAq/^#4r86&&v6[x%7)K/x7%N@("iYUmnWv<EO|"UHB[L7%[#T^M*7%EaFz`q%&>%%4p;9ELupRv`I[(CwR)l-JcoD*/{J)EDHRW
b}x[
jNPh`dV:uyRlBKZJ%>IqZYa1I+Rs;.]&r-CrJs9W[)Ero_gjWPJhY9q+xHN>ELz*rl5-[n[*hBo]d`4:tU6tMH%nipO=x%bvDrnGYfSvZ#|Ns&RPGY,TvqLtJo8J`PPrQd0j~smlcd"y"lWPK@hdg2L+O[DbO0g6j!1u0ev$dee:nsOhaK/`:DW]FJ-,QIHcyf?qgycw5BFv[KbtcI4K|PGS[b=Q&U+@djz0p0j!GO89(9x[-:>i=_p?]L;>9[O)}46hkNhbiUr:*vzeJxMehd,<vq
hejnDd-.%f27+cNitAe/149,IlTz/Lu`j;p5_Td:56k@iSN+i;u)-A4|l_uT]/h64[@$dI6tZ7Jpkigw#[4_]TX+qAJs9;H%2/(_7DlM]TSVRY=Zv=XAq.F6mN>bmgEf+nM|n5^Ej]X<EwRVi(E7UIVq&?(D5JSY)wWCBkn-XXkBeo
f^3c=WJI%Q(5,:QZqg;?.eZ@xAlerGY$y^lR35&7Qm=q/K*>`h28M9[U[5z,D;J
g#9kZALly].PTi]/DhjsN=~8I?xj?)Y9]J1;hb7Y1KLw4&A(/(S6O,iX+EE^wn4WYS7M9$L#:rqv*E#wA&vgbg?x6B:F;#7To/4T:Q"jj1SbJPmtIy?Ox<R1>12(hk]4mTx7kU,n}Nmn2uBT0$ahky#hH,osKQ7620CUv:JW
McH)/hPzaoYKMedtkOEngM:9rzp6rc@ZjrsRpN;ErF>mmOG2pSFY
O&{c!0"5*?3Txh3eF-|MBYi0b](J+r;jAOWcDSBlS#{;s3<S9i^
c2wW]y[_"P1Af%7[,W;D-4`-:SV*%g_Bf3)C[
lr$iF5{2#:g8b.2jM0zt&CM%u`k#;^16AoP#E^}S@Uc`>>u48xj&b7qiKQ+e;/EIuq>2Dx-%RW23Zf,N!vhOYG}KLu&kv1=D(Q7CuhXw0A)dJC=fDm^6o>p6Y$
E]XS[7P:^/%B%]Ae[qopj25$qQ7Ndqh)L}k9%FdF!DA7Nb7]60wWR!SGL09h#Tw5bZv3X<gny[)Ut+H0opDaCBHnGh.%s:0:&
@Kq7Z@3a]ucnPfMDjS#MNH,d:S2yKRfOFXo9bwK#B0gc7uFsIM>~V7%E[cr#O7L3aU`QOLL#B9NKnz-(Ureutlgc#/oLR}C[e!DnkZTiSXtPc$
M$Hvz[YvZaZaO$60*N-WhIis~w`!H:^phM*j^x!vXnUnnoF@>s
d-77:l:9M5;}"kt_"bqjAOuwR1Vw<%MLFQwv/g9G"f]pAGaTims*Vm;+,C"&pa?WtK0$[|&NS
3:0*_U8yH!(S))=APuPuSL8`D3EodjbL#68[/CW+5w<h!0.-[6wGLJekk*l@i58IX*75#|jDi7D?ytW-E|HWClyaJBb!y%BE/XFGnx>_.ba;(6vud[ITU2Kcg=#j_<FU:wTm)|u&2H4gH"II7#?G!Tw!HT>$ldaEXzh^roIyQ
F1
hO55#%FLvB.nSilwS.p@76Z7[0SD18?
{[p6q>m_M5Ma3]xZ$0h0Zen8(^WBegzAA6cx61!WJlQv.IpesKK8C&?/oQ-?#prw{xyl@c/>F.!L`.>#S4He1!@bYMd93a
Mr^jI!L}ACMD4G"C#(#xEDquht?d_3y82eS&X)`Y.%1&IH*)+q!xkM^B2=X~"8qrW*@fiwf&jE68g{l=6Gh^Fu76Av=^[9(6C~JC0e6<@eh1V{S((xCulxVLCo[:iAE=6|)5LLVQj~$$"/!,)c<adT#dRrM9B,BdP49S3|q"xy:4T
6Rs1TS3>1=^;%m!@0!o)K="z#z?^.0QNG=Ule-Y-`/
Wa7ZDg1nGM4Ea1:Naj5ZSH/kf]TA_=M.Z6(V3)H
9Rq8YL+w29TasO5TOMJ*&]v4hfyW-U1;g#ybsq//AO|q~ttp7)&K5Ez"d`WEv/Zj<-p1vv5YdQ)<;B34s/A`D?S2^b
/Z!2OFpAQSSkrmtqj)V]-dE/fA$Aez`$d-[U!tgn8ifqz$!wO=
%YBDWD!Ux(<3T>u[apbTmaaMd6a7,uap(bNniAj;@YAey!_A]f-WTCsAHYj&!UrY`$>Ahlgf1(b,7W!2t$JEdp=2"ym56U_.yU&3JO:B[=Tjo&lwy`J0(uW^SP;W{*^)liy:gm!=F#4TEv0B)&SN=C&"Ijs+!!oHN>e^kJ`vf]ly)L{G&T$(u,8G4pJvLqhll+BW~>pI#p0E8d?a=@Lssc0`t4w&8@nCB_9k
UaO/9~OJ`QU})(:y+
((Dyo[sq!R+=&>n^S8
pYL.:C-]*Z0#^^t_ClS.df)NFUA/|"o!+_eqGD4Bb*["SrKh=MQB`LOdwM32_X(*9@MfUe/WHvglIg39|+D2=/J38]n#O`=Fz:=x-s(E>Sq?0fkh6PMT[0vZlebDVY5KMGUeoP8tq,##zBOY~XEuhN{/aF_M8/wZ4"[w%5uZ(YtHguR$+"w8@I%N7tvSoxkn8bA9Hm3T~Op_
S6`4ry:^so,QOYb=?F?qck:d4b)r`pmB&`myLl(D!jB@;OW>;VnW@=i~p#/#"kE[0@<o$7*URaXk4HMqSOn$o`3#^`G#*vwT+F*_r,P!rZV/F;5uAmCVkR4T=RU~0CE2W3.pM.MnM&5<5uEo++[lI}g@Vmp[AfG_b+C0xSUn]W.6?KK]lfccj]gjlFO5o90pGgeJ7shGMg_d6O9?x{=~Y<(D`1mN*Bp5wZ<"2ERWMd_$FX;+?O$?>yqS.NJKpmF*3OMU"zc6KKR:]sq7+t%$fm.*H`U{u[3fqOcPgj_c_XYGu`aX"Z*f/<9;DcoP
,f_d8.#JY$x4Mc)"pvnY8?wRS(:K*JKZD#P*^WI-PNwy^
xqfYe7`$
phWp/#+e>(&t@ZLW9fnh!}F_)pS|lj,vt{Bfr?yD6}.AQ.az(6_UY"J8m=#I(1sB`tS@>69O9R90ddkOZ4[LJ[B`4_9lXf(q&0)-QTiGs|j5M55&ug5bbuu7M[-]_4W,lQQQ*jtla%tb#{m/)#Y;,gkfZmshlsu9
Rh5b.Au`FCXZA_t_Bdm"",/Bi/Gq
QMVmecX^`x*~gd>fsADEyjyfLEC|r_GTv-6tg*Xn&$@9>g&;
;m0kxRkhBjo/1XVAzSnJN9xh8pDW|Nl&Jg-8.XU,_DG^_I}4=lOr_fC^Kg$/[I;LaoBx80Vst8w(%Qm5{6_#{`W4RS/UV=cC^G}]T!XO8S3G9706=BYy`NSrq.><l`^3e%X<GP`q:7Zh8MS6D/RLCe{Ly${K#lz"n<IjiTs+5Ul.qM!.6pVG*>+?[Nva%P"T]r^@43@m|
.>3%Cr*e^k?LJj_IKl8Oo`9.1`#3Bj8qAqwb78OXH+Ko6#[@Q.HL(^hOlxQoTfD
%*%L~XH7JH:M")(MGXzM0)UxbVc]z2OwQgGak)E]
k6im>S=k!Bcj;jLr8b,541!v%ymgV1(He^a;e1L2wDV+XDS7va3x+{f4sI@WcKg>BEjl`4wQrnyg_YqWXh-)6xjE;Wh_49Pk8{xZ"UA?tC=6U{3!Be@`y>WE`mx=$D
%_&_L$,m5)pB#`rA=N_[q,9Zr]0rH>kb4GA!Qs9/P6{(!+im$KWIQA4
PXIC|J$uw9vre=mN#k<)`)ZyCSH=c4]EUp{?ZGHKr?.&
0V*jw/GN
lC4a3IfTB?|
|9=0kuS
}D#LqgW@4*DP_h;/X0tQ$?kjx%1wjLe2,
V#uftYK(8(D,ul;VRW<kRj
F0Gsx$nv#0B4bp
FAj
Ex9*=g05:OL0b`?V3BPZ/6=-:Ud/wX/;XeAi0(.%GgDT#_$Y}+K#$iHuZV0]H?:)Y73iSa$I"/qrFM^S79y@z5{SQn]_`;lF.dDEU[8Qe_boWr%M"#+RtVxoZ3]`*6ryICTu!YBLJp?Yuy+6kI?N%ZQ%Ni?,_-=Amfv)u$GnmX~[D<.un(M2"IN";Bz"c]U9Uxrss6}N.!OvZH
K`d(I;/TMqB|l0_VC][iH1-)(]x*V$;/:tg[8TD4;X=?[R[t1Az!F.;f6&8I$/yRJQpX8lt1Oap|Ol.Hqq&?P.;FToEGS,tStuY_$F&-@f>q_gx3&|CPFMci+Aj#=MXz=W^f"*oAZXqm*5;qvYe:!NmnfQ?6f"tWK+>A0@tHnerd:D)]m~<V
@cgU
`k=hATZAj|
{S5M>X5U/<9S$F_PThg]f.dj=^,=>3!#;;^PN+`Nh8E`3]<1lj=Wd*`&(p33D,#9YX
pts?j>-qEc<U!Yv8*OQ4&~p0d5"ckWdLcQMl:i#V&?)u"IuG@`G-S-o`t(a#T>^pkuI;FVX69K)>M$%(t@/ZPXPIUUNhB6%p?}w-q*<d@XRqcbc;#u2(09e6or0Brq(p0ve1_mCO/tlM<EI9KPHuDNY]F("}F#Ew.}P4E.7!:v!?Vzd-/U1s
1f*Bt"%nqJI)wft!rOK<pMHyPq_$f8|!&2Ub$%4YK/%,8b!?N8e=F63JX/sAFjR,?kvQMS-mb!MPwk2,J4jI9B&jrNRJt#1QYc<ttJ<D5qLf}j+78Hoo%K,TMM2EH#BrfqYo/e^]vdgF[^>P`SkgHehZT<Jr,_>N9a;D_9w.^bU)7pJYvbhf,C]OoQ1H|tg*OEYdN-ulU^ts].<f!b6/zq?Ek&Kz"+LK+5)kxnE`za!iFi;N9%TQcfSPcC2w5GF@69S`L"9lri<1:U^O`;s;-tA[HH8acI>Blc$VmK(g$?|bx?N4otnU;k3IR)rR9(sgp$I-|OsuE5tT:S)CNwn5(KiCLdDAp,nY6Aj7G].*7*@yDihL<cHU&3Uo7q/
0M)wdc^O32N35Q;vFqf4s^i@EfFA7lLvQWLFa?XCB1"d8)P/v(*VG"qAM_Ay5tFq~["Nn0,B-?O+R=w+/n~/%YbbYd+kq>M.#?8M(kIwT:Rp`ijlc?&Ge#(Rt^{meMm`JK%B*^7@&rqt@it1>Wr^R;EJNP<vNc=rHQIJL)#C:7U6&aI5v,S)unjR/vfc`k3fHB+E6kl#xYZRk6`!5?sF?+3-6LMJE<hF8E#rM<ODabid.>Rnb-c:!Vj`0%Q2:@2wX.&hB9@n+]O#x)]W&#}q11dbQnZ-#"/S9^ZBtwXio4hBhT}TG-Ddvf.5;2kF>5{A&f~jr1t&9]KO]?<+a4:M19@=~qAg}P].S9,g)V{hdY~Dr"T,pnjYn?aJ$"e*T=DF}3)66<o/(/XuA-wPH<O$gK%Oan"A9XiTPbzCA#!lZQ
u`yL7gD}2/H
COPPJ*5E%XrI:NtJs=P+Q9>?HM2O_`
5+3s}.TSzVaxb?+_i@cB,Q/s~.~X$&,.vW|Ws<$UVO=Si)/.uEVc5Metw=-#p[q2vCstCGQxvk9@`([">%0"I%*4o%,6u-C:oVOPwat@B]Q?~V2.vCVTn;&nd_eF$uM_Gs?KSX[pXy6!mg2lD_|SX&2;}<o_W=!lz,Os(1;p`v6O1T)SRZeoJu0M:;x%$jEKEr(t.DRw6
h<^t
o7v*AxeU#l?hC5`#_j3SX&"b;dv`k6;Ww`sY-zVP
8b@jCl8%Z%vCkFUb4T$S7u20r><yg9!CMiNVI5mNjlqySF4a3wMU8%FZIS6!po
mp6GukNw(/`F4M_<Kf/d37dCwcB/Da4JC;y81[T2l:PTa4^l0%LA`J`!-<8W&8nRnG;r
lu}2++
nm@ZK/$|5nO}>M#R:("O%8y9ScLkQ,7,8r1ux8IJeHwxFl_Hu@4/!|)xpbr%4Ct/Unccd/o}/wqWwZ#af:Au:UIjNv8L;Nl3&`[I2n`;n3
RWu(s
S3![]AN4nU`6HT>FRw#,w-Z
9=rb:_{vu5@"CswEf3IVoWp3GiNrk-IXRPp65?J1^FyKFM^(yT37Lb6bI`&e$C>oW
0r0Ztp&51Cf!!QV!c"}KDk{][(YW<&fr4-"&TYsB5*i=xF,^yZJbflQH~56+#f,6|g36;[o$Ytmq;Zc3rj,#d@CF^]tuCp,M0VG##Y}&X&I1MG,:0NzJh3G"rvOlZd=f.?#iSrHUsk1-7-i_76wj[qka[k#MAxc<fE(."f
h(!jJfUO93Cr,XI@L}S/
MvV:IfG43C<@TmP*r7X*qxnM%vIx^+|&y6jx<YU(HOiEHg=4SAHvH!e3F2YJ_y3B:"D/4a%hS#|N^!+h{,b.?j/rX?pf,x],P&bo##ACuuEEDH$;15.XYQUQ,uueJ:Ma0REhAhY^T1S9-P|;kqMu&:sJi8uUsq#5nT7k{08I,rUrfi&j=O{OY
qFqLl_s
r&`/SD|L)N~X&pZj&vy.tggvE]mFn!qU9O+f_nS[mog47!u<sez9jwwlXXQHE/@!*N,!4tZ1X1mp@k=
JlSh39eZB%nP7Q4xR$Pkzioi2JGQRmEH:q0DLK7*!=;:m2Fso-9E"+rGCGIH|E|Of:;Bk-ke;SGT(R>X0ZPfrqv@[_I
)=.VM^iu4o(_.(VW/&C:EZO6,f!BRfc]oVJx1+Pmz"V]Q#r2Pl5
foynVX=xf"*F<,A"cS0gRz&5,,Bpl:vtV/rxYNA:j.g%U`;7K01,Z8[M6_euD9LI))~PH=ZF;vv);<Ud;B!Ws6XO-;N.i:x3_Pu@o62b*Om?w%UOafZ#Fk@RwO6W61-^Y@U-~m51tXsd:$MZKQ./;O-EkiW$11g>c^^NN4j:,q2<X.X
"J;5J$n_0nemT1J7S2#/WWWoYQ?:A`dk><{-Fx4m^S@0KanE.>14!!%PoO?!!f(myjg4O2Q9$Dj%c9+JdLyP@"b^ZX9?D:s.FRIqkpOo!`!m
uH@[:|2{NUw*0$2<!bw{<#U2X3l"S}aJd>I-%1e@?|7r0L2h#Y24Ks.s0)PHG~9?Yx]Uxe2K2xcnR>sM&(Qbt}l8P1@MJy?icdWDh1HWPwY!A6@-y7<Ue8vCf=/5xK_ck4@^eR#2S:ch!Gf"Hn0f94$ku&t1>aT$[0$To}@9,:ghtgI6HTJ#B(=.2e:/.NUg;JmfN_My6YY^b0ggK*NA
>@+4KFaVv1B%f1
$1
gm<]xQBPaaw3bS^tE6wbPNpMq8V0zm-3UG7(v,+3#ApIe9yU4fIwP.gGiNk<=E8Q9_%+YPa_[@#pca-TZn@mOfqeM8Ip{!ok_;zd5ZK:Q]}eg+#?2gy@sYbx
EaA$>S@J;%rl=(iN6Mby3au2D4SkZhpzgOK
F[!sr!&Y2Gd}A8XcK0<MGp"49SO&$E-C_Z2C5}M2lQdzoD-ym}qG+/IFQcK3e(ts>l`:^/dkm>t<:h;knEUIszROK+6oG5k"_}Bb<^X;QtkShJf}iBl=do?eg>@qTmgA9kJ5[zS+[@={`Ifg_;aVyo<u/Fbw?h@}R}+D]L
Edar_/GBww>d5]i*9;Nt3+vun%,Q:#U$W74D{;a%sFK(CjNw/8w$b/z%G^A*|a=S`6*G<[z2
YoHQWG+CngL/t&c8i5ZfIR@o*QVt,[(.nAwH6Zc66SYWCRE:4PY!bxZlRd]DH"9A^cJ!/9vqQ,mFKSXx4@R=aRfPyOnh93)^i3<)A+[J!:Fa1$T*hM?PqO[}mw,[_v0qcP@<aOR"^^uQ>g?o9Iy#y*n.TU66?,N4p)Xbr3G1tS)R
;tHX]Dsj>t>!0X9W6mqK>$JFel_[`v7PY;Yy,]CXpqNSJB@a]3DJZ-0MSl;w%;q
;y@I_Hc?Ebm7sX%BR$)nzt$K<TxGHP]fdPgVF^<B[DU#hslc>[J]#Qd_}@W8%B=/lEeGZYy)VQ,3GqhCO6;*^jn[f_~t6(WJCJGVxjcd^N|f*xL-5%j5=]FoIb/dR]yG|ML>m4(OQGN&nfbg.T2T&.U3q;n@|NTmLg5Vi
oStZcIaf*uCIUDxgDM@!bnnI`Y&:pocaVp3<34cvFqp7sX"uY#2H@*[M3`(Z).x2FjT0iq**4K9oq0OD[olyk2&8;Ithq4hkqF>SzoWN.M|D;3|ViZQs=XCa6p5&Vp9evlE+2ui8DdOF1]qg9sU%rVxTkEVHEEqT4cN;+xdvj_FI>Yt
|pta9l1.;hK*9/ltY<!fa1
(Torb#[NUiir
,3mvgg#]/HA2#;fm_PCy"+qvunL!l)vyfJ]])qlsZ.B^qQ2UcKZ)EQ{Xq2ht,v.,(c&X}A^&#>)GY=K4
j2uiqZL7/BEWAz[,^}+8&193S.LO3$_%sZ/d[|:rG^EEBPnfNNKN7;_VBAoM!CP>q6/kIAQc)r0/ofKb%~gB7{/BRKT6Cy:IVLh((*4+9PK5dxkZT0*MK^N:,Rs=H&8]66B[uaHc9tj}L#0y8*,-p1;&_HC
ne#&:Di&#&Qq8]l"s[qW64a@pThI4Uphox>LP#T9@>$uy3?^k0h#xwaQ31#K7OBW$+u{YZpbLrV8<|KO,~6CpJv
p*^QV=t/Qzx`YtnJ<"a/2M:sPC,yN&/m*fO>y*L_%cg?0L&*V.%)/<nBQN%YN>!pN.5{5&[M-EVh&>mnXNGryUu(7)=
.q@"RXM3FY&1B:0QmeuwvGF=4VI7*?9Uw4m~;!/-=snWP~=^p>KC>f&#J:VI9g,|e891ywSDXx&wRPi8oS_7I=>AEMribq=f<Z&-`"4,Jj5)-w56H/&+dUA*^7Vo.}O#^q@~N&HfY]l@L+rRTdbzZ&I@C#jpBv5H@a))Cz22^n8>^2L1SJO@=AQrsXh}!P>sww
i$
uvt@q$)^TC7k=uxzQ>n4*zf$g`qSgH^
g`=z3e>OmLMUN3eB!f^9kbYcihG365i_b!->ypZ1Z!j."l.kWT`"95Zm`19L/n04"*Pa<oQ#ssGlImjl"bNsS&_tZ@q~`m/DBh6uY=T=E]VvF~M0JMj9<dCdXW`slX!%6D^ai>y(qnyrf+BX^GP]j~%YE5nRrJW@?.cv3BaTULA]O"SI$3!{d["*g@bNd=oWm]nI2NP$cR7}qMO_4jQ*")Xl/x25b~Q_"1t,WDcd*,p_e<Fp7hP9x!r(2KS|$!c,(E3c5~P)!#BP_T0/[~M+#u%{D]d1ZHntjp7k6p[bLQY90"Mtg(%&Y#5bNP]iD_X~AasHZe!Eq]:IZC(mw1mB
_D4BWXnz&r3$!4herMvQP3w3B9gYZ@;1543hR"9)d4|Y?eLCaV{3cAm!W>A9OUKg8JbYR&h);y8HLE-amuBq_5juVLY8[b4[PPc7:gnoRu)R93aVJ`ke1ADp4K?Q9<a^8$7YV]>mwY7/27X;-?)#0
V>W(~2/Z@c8${h@Vt.yvJ[Ykqo$-ZLr;lkL#oHNpXsB*qW4ebydA{M2yUYA&iA1Kf;DRhjXc0c=o+%g&8A#q?GuPhGWvx3]?xM;Ufw,ouB)m29Gji>a*-`s@CNRYmkxmL<l-UH(1tv-.A/mb<p)G}j%o.:#bF"90`s<?;Pol1v0^32X+uirN^cXSe4atH>tJ-)qZHheXlwe:3nYM
!DYzh@;EHP"$X7lj3PMlR~ghMXEoO:1D&{Otl$!N#4"`AUe
SPmE[~6{/fOY*n],Ii^iI7gbBa3{_$pBw27KHa"T_tbzpGKyi}R$f1y.ab
%onHu!<t=v<H1v)7ZIUws)mwok^q0LO*"ZqN}T)=rr7W~a01`[g<}qVuwz$C9wJQhD6T}&@[h_JStWY]@gpeJ_N?EG:`$*;O#9IAos!fQ$M-<(4FqoC`0FmHH7
KOK@+P_oHws
=DwBg{3/YWFj
z-*lq_{SOpK*b5?6;RtC}h&r%OK`gkK)WNT"7kkjP=L4_qR,$>V0CODKt5@B<QC_OT$aIXw.}uS)h
@
zmjyL6QrOAnbFpK,irpS&u8<Ws4vw5cK@[|AqMT,/-l4(.O6V]=Ie`:DN.4%oe=7[fuv_
7@DtEP)TtaVlx^jy{fPE$?fku$W@Ke,Gq^Gkc6@RTLalAlulK4JIJ)=gA@B)|F#67T]QIBBoQRX8kUSN"S@o%i4RRMf`+D@v[/!K![ubrvDiw!7Xvv3fdesDGVmtCwm)zsp7?e/u&d4Q.&LiH)Y#$X^=tE
88eHf2?U/7G-#<*-N5qxB{+|E,DIBgT<$_hIv6y!w@WIS]+]Hw,)iI.XOYIeG[#$oMudH%5/^/PeRAU"6s[j$vf;4&6OpD"?lXmEhk7:^_O3W%9P5b$GR0SksJlAT>i7$@CPClVdd@R|69U6-o0O=Xc$<as<F+&[LYn(3uji9L.T
G(Atv
uAB9?+
k`yQ,EqRn;^TNsDMqX[*N0M|I?fr9<#7YYA[$D]w&e7!rzN.Ksx-wVgs>kUI&=V6_Rs$/L&M>_QhMUoRoM3RD
.+%z`OBL9xUu+LvB:7y5f,.gM~0
&0n~QNt^H<&_%r9
S!7>,`>BLcJmC&fL5D^c=Ak:!?RQN64{hiE$y"I%3xyul{ALh-FTuk=HY:`R7v6.(A$U^}rVcQ0C#!-LgsAF!%P(O
:IW@C)p$mtS8O1jS
)w5)!KxG+`S#,!:,+Z]"^^&ZRs5bAF#lR;Cmqf[a+a#pCbcmmBR]V#A`@gM@);,IZRTkgIC/>?Bo2.*M{qrDjn!XncC-X;tImUH:P[eMv*N%fq5^6;Yo#$&Ey<]PH`Lla.B-ue6@d;Wr6btm?DLh{=?[+"S%)!^8.CP4{*$c+JAq/@;B[RX@D$jv;sS=sVJd(1;jC!b_-0gI&Ir#Lye2VGNdoS>2=#d1M8i[DwnSHE|jj<(9iL8-vt6WC
HDBqV[=n@V:e^Mz`wI0YhoEoDloI7ghEj/Y[dm)&`Kjrcm-3b4j6ZNv-a>p?2t_BS/`qq#70%xcj@*)M`j~71t#E]-[!x(WXc$u]s-VO
okc,h8i(anfTM+R,Kz@]K88ekLY)^tt]-xM5KlK+2Cjidf
r`Ps-,ZT4,3mfRq[~Wz2$&?WLYs>eI0udK`h9mLFWv:0c5zG1[jA29Pv>V?CK)*sVY@,kIo+r?v75]EgErO
xr}Ap=$ke:Dh/+<GXJ.bXx}sJTBx/Zb/t8[+<`Zd_Vu$!*b)4F=W5sYRn:DW8[~jm"^6GOU)i/7E>+iZR]>jD[Z54F$P1wZ6-u+*;wP7}:_da?9-XhFFjV#.#-GAz4rVgP48]!)G=Jw68<1JlWaIwY,YIU
ql<F(0,AW~YB>SHMczR@TOK-lVFv!mAo/~>Lun,Mty3TNwf4rfv.Qx]F3^3r5X2:/."/*{
6H.o9=)LuCpW3+)U02QgqVWV,nnFI%Y,9x8&LS@c,Oe;~MM.#F#mYPw>QPc#-8qA!H7S0WI6$4kBb.:J}4QpGvsN[E;:~:^<kU(va)#y_S}6Rn|mpTv
]#<-y
6x%p7"asepyl_#&T0cZP2J=T]4274EWGe)Rp]fl(,?2NHaHs=MUa+6Ej|u<,F
X+?,BFx8E@D1gZ@.{@@:,;sNwQf-K;Ut,+lEaGFYdvq=<tUQ{#~?gB^ADC1G~@
=jylp)!AtR%[/[:{rsF"AAKxnIf9:hI~b|Vp;,wzV0L{vl;4/hT;I+k:nfrEQ8*b5{QP0s5p#jOp0;d_qO&dI1cdPx4p33]G,Jm`t8d,3lrdM[,akTu:vB25X-.0;h"4/s`4<<*_Sst;=a=WtI$xm4[9;:$>w]20/q)h=V-`c
dW9+wvhQVRejm4%7t`M$C=$I)-Y$[dEN>B^W&q9;`,R$U$^`m|ENVu(y@
F?MV6j^dK@qeW7C.s%1y]d^&,`lF*,vVGo6ZidCsTgAAZ
^r9!K}FT/vuO#$QTXB26p7#l>r%b+g^Y&h%U6>Ocq,Ukmf>m-z2,o`.)G^eWoSHx2W=4)sQwn}dc,u
@OSt@/":OC/U{[_K+QS??V>mB;a;PQdjSe+tJW2Ax`Qt(D5y1j}"9x/Js?mx.2iaNco.+Xy[v5QRyZf.NPd>CQ|[Hao1U@45dYDPd,$DGRzSU9og+maqy$`v>A#Eyv{:NN
9L0hb~m}9<8W+l=BwSG*R?M?oY4vb
rMFPrq^@m7lmU0/&l;C31n15X8.O=QQX_3r#N@1x^5uQ
~r1u7Xei3QQV_>{^K1UFKR}l5x
y_1["]`2^%qx3wYC`iYU7<$fi7Y;Tf09)yQqf7(dn^T-Rm(7nl.q/Bu6Rh2wS8acKP/Jq5slKeFf4[eX`@<*k&&fr<@ne<p@`ljh/iGi<6Ri>)MYX5:!*fCi(t3xc6G$336=w:fex7jP:Hxa(;p=msv7/M76v,$YpUa5j=43v.WKn&fibIi@/bZSuA+]Uv(`E@3j!b2g6v@F0&H"w
U)KCA$sfd]f_HG_~`PjZ&Gy,?|9W_a/i,S+0a8pwRS[!X_E9(/w$#"Zc2Z0f2fxh0@f&#MkJ_?1f0MLO^,7$JM`#qdYlQoHao5`2jEfti]@h8vd?^LC?!z1prsBUW>:MOM;C>Kq:E
=RT"[W#M(Zr7gW7=@)Qg2-:y*wMEYtxdY=34M@lXEHMoP31"UzAUXp+cj7AloA2"/t*RhB=lJto_j*AI6+lpv{(fcziw^+If6_BrpHwf_<BibDu8(7=/ZLZ[7Vh!CC_*degS7zWH%3wEEWf?v2@!$@+h*zt-tp?US
*g]QTW+xpjg6g<*Uj_k)Bv
D$!V+[qS@)X5k_CTxs~`]=}WT">Ro2{6;axZ;Qz[Ib%9Iks&2HF=Mnn]mGeGabe]7YNPqgzU0q
M8A^COOC!8vN?
IA>{?KC~PKJv5}NHAh1OMjx57k-O^/xw&c%W%K8;87TinU&fqdNfh5*.ib$u-Cjxu..Q_E%>]A>@v?UaFoEE*bJn
-aQ2|l[FcG)LrwCqKp78^MD%v0g,%GhIR:n[](yFKj,%|R[)+fw6rEJ?3HN%q(N9YS.+C$*&9TRx*y~w.[F^a$aidAXLcwNQwO{[bW4fJ-.O[j&ZfKI7<J2dB&KK.D6=rJYQDwy8`p(Cb2P@<d`CZR)w]
b>Aq{@CCAP^?|4]N5/PR=sk;+?.Jq,GQf0PbEHN3!:z&%beU8S{I[[YN?"Iq;_{yQN0_`gJc}.i<nO!CS0dT*`G[/@Aj:Nyhdnb3`_B_q?s
Tu$$%Ymn(e~aZ2*.bv<OD(X68u8M6BgB^;t<j[Ku%U|jIwtJ;LZchazrKV3-mOzhPTB@}/Bs2>{(U`3Wpj#uspYWwo0Y62R#af-3v+DNRsA:})u+u3et(ACIh=Dp.b;P54uqu]oS#PEVe(N_f$x?A/DGJ<EJb(.,J>]9r[{KH.xuiE:)yQcm#@6pO<O&7,B5~<6>Pf:c#*_j@bn))H!l03JG:JW1bq}JcZBx+2_iJ@4(c;LVHerieE
q}%58/ptBGI-Rr%AWy>
yH@c#G`c)>m:<y>pl[.i0&v_a4e8q)<
@7UQTtl{M+q&okoj5^+!)l<9t>%uJ?5B^O_X1{2e+Zuv];r)3Spo!_2(bX?|63U[RF*R4{-tVr-IBzV#DK<%F&t>n:BZFo
rO%PhicwHC"_@@YN]s<gJ`X&`/@L|63<-mjR_4nVf304lXbERUEiMX[>b?lT=Z.gM`Q)gl^jA%D?t6wxK_54Rg[.|Vwyh^"s*U|#e/L_|LCK]#`BrrahlIz_Px4+*up&#jS,VrC.;#^)kWar3u^
JarU!,v#9
yc@*#@b5Y(S2J8m3OK%5I7,A5Jasi3JRacNi"i?v=,a=/VB78ORii<p2:$yV0p)Pi3Hc?$%f+Ynw@eTd^5WCIW@U}u*KLJP2x=s=rT`8^w%a+F#4-9x9HY-kOj)%j^@6y2"(fTg>MQh)5_H_$NdOmwc1
Q~gU2d2!lfi*kF]}jRc:vIah>3/Yo1R$l)Vawf7`Hw>3MQ0sI1Sa2{f0;zQ"+T+VF<cdC87u]P9P?xbDdT[)7axQ#viX?:y7E_?/HJHHE=B0McxOomGyOuBzndG_;Ct`26IB*E<06moKI.4gIa;Hb37$xAS2s3DI[iOfRQg~/z)?r1/.>$WVF5hBZX4<DN&.r+]io(rU_kv[vd%24Duh:xkRD>s5HYgh>(=bq~U."37a[m$SZ~.y=MZdr-8&xFqu)HO[_DGs=nKUH!Tu3Dpi:vvN3Hb]Uk0V7A#1q=3S!c($oZY#gdT(%Oh-9C[d4u%!F1=3^laHX1p%q%n@iPo|>*Bq%
A*:)u%HJ]J&b!Wyl@F*)Z9Hv)Q5e/?/""=UORNo75Dq%^iG?(_KvmZO/eBo.fC-V++I(xds73_1pPwie39Q;^-Plq9oy69p51uQR/`N%rsZ2`Ntx]DYO<K:dL%ACf+?*EuK%`]CU):GBi*m4x/aMnhmTZ3,>m^xkZvT#*LHwL@DD6yjT]TCQ(GZYYR^WH[J(&(y5)CD<.noj[j&#)5RE&-&qn*"G10iss-Ndp!LcYo<4v"DMD!ZmT.8"",hEox_^CJb{5B@CRd-"
;E6IVV2yi5*TdCM/3Y>L5/87ieFtcbgm*/GP{(0[HrHHMQ[
Xy+4(54x;5.HQ*~fhqD)d7!LukIKvHD3ttvD.4<ke5{W}2Ah5"jct,`lR/GsACXyTyWnP;#y=6OmH_m
Z&GpxZhBRVy<pyt^Wtc?`b[3k&+j^S[:@4<JPfcEqfv+bk"awklD=qZBxlV*3fXihJpf
qRHONMv<ijn~Uv/M4D1l+3.Iw<p9f
4RqD0=Eml%F,Us>0B}k<7o46s(c{Tt0smX)(cCE)l1fA!Ytf:6(Z7+uso$/e<Ql-,*]dW`IjI:>Xrq[z?CjWL`$~[kh}hHWT,x,BT
j%MW:LMW&WTcRfYK>J]wA&Jo:+,
)xD<@yaiF$^
$(vsfBA]C/PfypnuuHX/r[cq8M*;O|D7dWR@Y(-.Jg"GdJ50@+J&0t
Y9LM,aw[oeI6UxjSLtCC?*[<8^h+;(I_qr;b-_7ch225v<8,PVO9n=-,<lePM3BFD"R"i_z-^x!GjZ&[$J{X--TtGEp-d"qP~Y0YJU/Y2W]MJ@Y^oSJHfQDIY!A-aN?Q.^^h8KES$sA^Yy1C&+ICkJ;8*gSf[#NL-&Vr?=(%Xn}B9AIJ{#)J}B+^!,WhnPOOTdVF$#ruQZwNQJ@L[;;DNY-sd>++YHJRQR5J~C%(EKSOVByLHRU_nhFZFEwx@xVP{xOvLd>KJa!$c_E?[64#x.7.2AP_BrY]l=`s{s$IbUypKDZFX%6:#"(Vx4=6nMsMo^LJ02pP2?0Ebsj"Q$&0bL]@00%5CYHRA(
K$?#sV_Hu1CAI#!JvX@ZapD!k>o9N!CQjabl87.~Zz;t"hVF0b:kEW%f7)G,`hF!pb9`!"-!`{4Ft)2EdifW7pXO%qf
M:3ch)A:mZ9,]^R9Z542u&(`56u?`K@L!1)M_14Db<x[S<Km+JCX/Z
^XiN-9PqVPVmPg@A-A(SYXtwe0FSBe.3Vya&GFVf.-FbG_"#}=2Du_2B&<"^Rf{;13!R`[G+EQhZ92DNabCdS$*L-j&uyl.0~l[(&EF(v$SZ{.sK"mm4k%aX1`<q
.5&Hj=X+m}Hp0il:+[Dst;4i
NN&v>SpG6u%0;^*AfJOQjo3XE7"bl"sd4^Kfl5#nF&S&^6"4oaS"4s,Pkhh^"W#

&u=[G%[IcJN}*Pl+Rt#dP5u9l6T*q.T[7#`+rbO)iZ)bS"wNU}Tp!]Jy$5s.]|KRy1b_o(,`';break;default:$f=null;break;}if(!$f){http_response_code(404);exit;}if(in_array($Od,["png","ico"]))$f=base64_decode($f);else$f=decompress_string($f);echo$f;exit;}if(preg_match('~^/[-\w.]~',$_SERVER["HTTP_X_FORWARDED_PREFIX"]))$_SERVER["REQUEST_URI"]=$_SERVER["HTTP_X_FORWARDED_PREFIX"].$_SERVER["REQUEST_URI"];define("Adminneo\HTTPS",($_SERVER["HTTPS"]&&strcasecmp($_SERVER["HTTPS"],"off"))||ini_bool("session.cookie_secure"));if(!defined("SID")){ini_set("session.use_trans_sid","0");session_cache_limiter("");session_name("neo_sid");session_set_cookie_params(0,cookie_path(),"",HTTPS,true);session_start();}if(function_exists("get_magic_quotes_gpc")&&get_magic_quotes_gpc()){$_GET=remove_slashes($_GET,$fe);$_POST=remove_slashes($_POST,$fe);$_COOKIE=remove_slashes($_COOKIE,$fe);}if(function_exists("set_time_limit"))set_time_limit(0);ini_set("precision","16");@unlink(get_temp_dir()."/adminneo.version");class
Locale{static$Languages=['en'=>'English','id'=>'Bahasa Indonesia','ms'=>'Bahasa Melayu','bs'=>'Bosanski','ca'=>'Català','cs'=>'Čeština','da'=>'Dansk','de'=>'Deutsch','et'=>'Eesti','es'=>'Español','fr'=>'Français','gl'=>'Galego','hr'=>'Hrvatski','it'=>'Italiano','lv'=>'Latviešu','lt'=>'Lietuvių','ro'=>'Limba Română','hu'=>'Magyar','nl'=>'Nederlands','no'=>'Norsk','pl'=>'Polski','pt'=>'Português','pt-BR'=>'Português (Brazil)','sk'=>'Slovenčina','sl'=>'Slovenski','fi'=>'Suomi','sv'=>'Svenska','vi'=>'Tiếng Việt','tr'=>'Türkçe','bg'=>'Български','el'=>'Ελληνικά','ru'=>'Русский','sr'=>'Српски','uk'=>'Українська','he'=>'עברית','ar'=>'العربية','fa'=>'فارسی','hi'=>'हिन्दी','bn'=>'বাংলা','ta'=>'த‌மிழ்','th'=>'ภาษาไทย','ka'=>'ქართული','ja'=>'日本語','zh'=>'简体中文','zh-TW'=>'繁體中文','ko'=>'한국어',];private$language;private$translations;private
static$instance=null;static
function
create($rg){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static($rg);}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}protected
function
__construct($rg){$this->language=$rg;}function
getLanguage(){return$this->language;}function
setTranslations(array$wm){$this->translations=$wm;}function
getTranslations(){return$this->translations;}function
translate($u,$B=null){$u=$this->convertTranslationKey($u);$vm=isset($this->translations[$u])?$this->translations[$u]:$u;$rg=$this->language;if(is_array($vm)){$G=($B==1?0:($rg=='cs'||$rg=='sk'?($B&&$B<5?1:2):($rg=='fr'?(!$B?0:1):($rg=='pl'?($B%10>1&&$B%10<5&&$B/10%10!=1?1:2):($rg=='sl'?($B%100==1?0:($B%100==2?1:($B%100==3||$B%100==4?2:3))):($rg=='lt'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B/10%10!=1?1:2)):($rg=='lv'?($B%10==1&&$B%100!=11?0:($B?1:2)):($rg=='ro'?(!$B||($B%100>0&&$B%100<20)?1:2):($rg=='bs'||$rg=='hr'||$rg=='ru'||$rg=='sr'||$rg=='uk'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B%10<5&&$B/10%10!=1?1:2)):1)))))))));$vm=$vm[$G];}$vm=str_replace("'",'’',$vm);$Ja=func_get_args();array_shift($Ja);$se=str_replace("%d","%s",$vm);if($se!=$vm)$Ja[0]=format_number($B);return
vsprintf($se,$Ja);}function
convertTranslationKey($u){static$sd=null;if(is_string($u)){if(!$sd)$sd=get_translations("en");if(($s=array_search($u,$sd))!==false)$u=$s;elseif(($s=get_plural_translation_id($u))!==null)$u=$s;}return$u;}}function
get_available_languages(){return
array('en'=>true,);}function
get_lang(){return
Locale::get()->getLanguage();}function
lang($u,$B=null){return
call_user_func_array([Locale::get(),"translate"],func_get_args());}function
get_language_options(){$Ra=get_available_languages();if(count($Ra)==1)return[];$C=[];foreach(Locale::$Languages
as$rg=>$T){if(isset($Ra[$rg]))$C[$rg]=$T;}return$C;}function
language_select(){$C=get_language_options();if(!$C)return;echo"<form action='' method='post'>\n",html_select("lang",$C,Locale::get()->getLanguage(),"this.form.submit();"),"<input type='submit' value='".lang(81),"' class='button hidden'>\n",input_token(),"</form>\n";}$Ra=get_available_languages();$rg=array_keys($Ra)[0];$sj=null;if(isset($_POST["lang"])&&isset($Ra[$_POST["lang"]])&&verify_token()){$sj=$_SESSION["lang"]=$_POST["lang"];$_SESSION["translations"]=[];}$pk=($ra=Settings::readParameter("lang"))!==null?$ra:(isset($_COOKIE["neo_lang"])?$_COOKIE["neo_lang"]:null);if($pk!==null&&isset($Ra[$pk]))$rg=$pk;elseif(isset($_SESSION["lang"])&&isset($Ra[$_SESSION["lang"]]))$rg=$_SESSION["lang"];elseif(isset($_SERVER["HTTP_ACCEPT_LANGUAGE"])){$ta=[];preg_match_all('~([-a-z]+)(;q=([0-9.]+))?~',str_replace("_","-",strtolower($_SERVER["HTTP_ACCEPT_LANGUAGE"])),$_,PREG_SET_ORDER);foreach($_
as$z)$ta[$z[1]]=(isset($z[3])?$z[3]:1);arsort($ta);foreach($ta
as$u=>$Ej){if(isset($Ra[$u])){$rg=$u;break;}$u=preg_replace('~-.*~','',$u);if(!isset($ta[$u])&&isset($Ra[$u])){$rg=$u;break;}}}Locale::create($rg);abstract
class
Connection{protected$flavor=null;protected$version;protected$affectedRows=0;protected$errno=0;protected$error="";protected$untrusted=false;protected$multiResult;private
static$instance=null;static
function
create(){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static();}static
function
createSecondary(){$d=new
static();$d->setUntrusted(self::exists()&&self::get()->isUntrusted());return$d;}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}static
function
exists(){return
self::$instance!==null;}protected
function
__construct(){}function
getDefaultServerName(){return"";}function
openPasswordless($N,$V,$F,$ql=true){$Se=Admin::get()->getConfig()->getDefaultPasswordHash()!="";if($F!=""&&($ql||$Se)&&$this->open($N,$V,"")){$I=Admin::get()->verifyDefaultPassword($F);if($I!==true){$this->error=$I;return
false;}return
true;}return$this->open($N,$V,$F);}abstract
function
open($N,$V,$F);function
getFlavor(){return$this->flavor;}function
isMariaDB(){return$this->flavor=="mariadb";}function
isCockroachDB(){return$this->flavor=="cockroach";}function
getVersion(){return$this->version;}function
isMinVersion($in){return
version_compare($this->version,$in)>=0;}function
getAffectedRows(){return$this->affectedRows;}function
setAffectedRows($_a){$this->affectedRows=$_a;}function
getErrno(){return$this->errno;}function
getError(){return$this->error;}function
setError($j){$this->error=$j;}function
isUntrusted(){return$this->untrusted;}function
setUntrusted($Pm){$this->untrusted=$Pm;}abstract
function
selectDatabase($A);abstract
function
quote($P);function
formatValue($Y,array$k){return$Y;}abstract
function
query($H,$Fm=false);function
getQueryInfo(){return
null;}function
getWarningsCount(){return
0;}function
getResult($H,$k=0){return$this->getValue($H,$k);}function
getValue($H,$Wd=0){$I=$this->query($H);if(!is_object($I))return
false;$K=$I->fetchRow();return$K?$K[$Wd]:false;}function
multiQuery($H){$this->multiResult=$this->query($H);return(bool)($this->multiResult);}function
storeResult($I=null){return$this->multiResult;}function
hasMoreResults(){return
false;}function
nextResult(){return
false;}function
inTransaction(){return
false;}function
begin(){return(bool)$this->query("BEGIN");}function
commit(){return(bool)$this->query("COMMIT");}function
rollback(){return(bool)$this->query("ROLLBACK");}}abstract
class
Result{protected$rowsCount;function
__construct($lk){$this->rowsCount=$lk;}function
getRowsCount(){return$this->rowsCount;}abstract
function
fetchAssoc();abstract
function
fetchRow();abstract
function
fetchField();function
seek($Xh){return
false;}}if(extension_loaded('pdo')){abstract
class
PdoConnection
extends
Connection{protected$pdo;protected$multiResult;protected
function
dsn($fd,$V,$F,array$C=[],$zb="PDO"){$C[PDO::ATTR_ERRMODE]=PDO::ERRMODE_SILENT;try{$this->pdo=new$zb($fd,$V,$F,$C);}catch(Exception$Fd){$this->error=$Fd->getMessage();return
false;}$this->version=preg_replace('~^\D*([\d.]+).*~',"$1",(string)@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION));return
true;}function
quote($P){return$this->pdo->quote($P);}function
query($H,$Fm=false){$nl=$this->pdo->query($H);$this->error="";if(!$nl)return$this->storeError(false);$I=new
PdoResult($nl);$this->storeResult($I);return$I;}function
storeResult($I=null){if(!$I){$I=$this->multiResult;if(!$I)return
false;}if($I->getColumnsCount())return$I;$this->affectedRows=$I->getAffectedRowsCount();return
true;}function
nextResult(){$this->error="";if(!$this->multiResult)return
false;if($this->multiResult->nextRowset())return
true;$_d=$this->multiResult->getErrorInfo();$this->multiResult=false;if(!isset($_d[2]))return
false;$this->storeErrorInfo($_d);return
true;}function
inTransaction(){return$this->pdo->inTransaction();}function
begin(){return$this->storeError($this->pdo->beginTransaction());}function
commit(){return!$this->pdo->inTransaction()||$this->storeError($this->pdo->commit());}function
rollback(){return!$this->pdo->inTransaction()||$this->storeError($this->pdo->rollBack());}private
function
storeError($J){if(!$J)$this->storeErrorInfo($this->pdo->errorInfo());return$J;}protected
function
storeErrorInfo(array$_d){list(,$zd,$j)=$_d;$this->errno=(int)$zd;$this->error=$j?:lang(123);}}class
PdoResult
extends
Result{private$statement;private$offset=0;function
__construct(PDOStatement$nl){parent::__construct(max($nl->columnCount()?$nl->rowCount():0,0));$this->statement=$nl;}function
getColumnsCount(){return$this->statement->columnCount();}function
getAffectedRowsCount(){return$this->statement->rowCount();}function
fetchAssoc(){return$this->fetchArray(PDO::FETCH_ASSOC);}function
fetchRow(){return$this->fetchArray(PDO::FETCH_NUM);}private
function
fetchArray($sh){$I=$this->statement->fetch($sh);return$I?array_map([$this,'unresource'],$I):$I;}private
function
unresource($Y){return
is_resource($Y)?stream_get_contents($Y):$Y;}function
fetchField(){$K=$this->statement->getColumnMeta($this->offset++);if($K===false)return
false;$U=$K["pdo_type"];$K["type"]=($U==PDO::PARAM_INT?0:15);$K["charsetnr"]=($U==\PDO::PARAM_LOB||(isset($K["flags"])&&in_array("blob",(array)$K["flags"]))?63:0);return(object)$K;}function
seek($Xh){for($q=0;$q<$Xh;$q++){if($this->statement->fetch()===false)return
false;;}return
true;}function
nextRowset(){$this->offset=0;return@$this->statement->nextRowset();}function
getErrorInfo(){return$this->statement->errorInfo();}}}class
Drivers{private
static$drivers=[];private
static$extensions=[];static
function
add($r,$A,array$Pd){self::$drivers[$r]=$A;self::$extensions[$r]=$Pd;}static
function
setName($r,$A){if(isset(self::$drivers[$r]))self::$drivers[$r]=$A;}static
function
get($r){return
isset(self::$drivers[$r])?self::$drivers[$r]:null;}static
function
getList(){return
self::$drivers;}static
function
getExtensions($r){return
isset(self::$extensions[$r])?self::$extensions[$r]:[];}}function
get_drivers(){return
Drivers::getList();}abstract
class
Driver{static$EnumLengthPattern="'(?:''|[^'\\\\]|\\\\.)*'";protected$connection;protected$admin;protected$types=[];private$userTypes=[];protected$unsigned=[];protected$generated=[];protected$operators=[];protected$likeOperator="LIKE %%";protected$functions=[];protected$grouping=[];protected$inOut=["IN","OUT","INOUT"];protected$onActions=["RESTRICT","CASCADE","SET NULL","SET DEFAULT","NO ACTION"];protected$partitionBy=[];protected$insertFunctions=[];protected$editFunctions=[];protected$systemDatabases=[];protected$systemSchemas=[];private
static$instance=null;static
function
create(Connection$d,$ya){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static($d,$ya);}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}protected
function
__construct(Connection$d,$ya){$this->connection=$d;$this->admin=$ya;}function
getTypes(){return
call_user_func_array("array_merge",array_values($this->types));}function
getStructuredTypes(){return
array_map("array_keys",$this->types);}function
setUserTypes(array$Em){$this->userTypes=array_flip($Em);$this->types[lang(110)]=array_fill_keys(array_keys($this->userTypes),0);}function
getUserTypes(){return
array_keys($this->userTypes);}function
getUserTypeId($A){return
isset($this->userTypes[$A])?$this->userTypes[$A]:null;}function
getUnsigned(){return$this->unsigned;}function
getGenerated(){return$this->generated;}function
getOperators(){return$this->operators;}function
getLikeOperator(){return$this->likeOperator;}function
getFunctions(){return$this->functions;}function
getGrouping(){return$this->grouping;}function
getInOut(){return$this->inOut;}function
getOnActions(){return$this->onActions;}function
getPartitionBy(){return$this->partitionBy;}function
getInsertFunctions(){return$this->insertFunctions;}function
getEditFunctions(){return$this->editFunctions;}function
getSystemDatabases(){return$this->systemDatabases;}function
getSystemSchemas(){return$this->systemSchemas;}function
getUnconvertFunction(array$k){return"";}function
select($Q,array$M,array$Z,array$Ke,array$D=[],$w=1,$E=0,$xj=false){$Sf=(count($Ke)<count($M));$H="SELECT".limit(($_GET["page"]!="last"&&$w&&$Ke&&$Sf&&DIALECT=="sql"?"SQL_CALC_FOUND_ROWS ":"").implode(", ",$M)."\nFROM ".table($Q),($Z?"\nWHERE ".implode(" AND ",$Z):"").($Ke&&$Sf?"\nGROUP BY ".implode(", ",$Ke):"").($D?"\nORDER BY ".implode(", ",$D):""),$w,($E?$w*$E:0),"\n");$ml=microtime(true);$J=$this->connection->query($H);if($xj)echo
Admin::get()->formatSelectQuery($H,$ml,!$J);return$J;}function
delete($Q,$Hj,$w=0){$H="FROM ".table($Q);return
queries("DELETE".($w?limit1($Q,$H,$Hj):" $H$Hj"));}function
update($Q,array$Mj,$Hj,$w=0,$Hk="\n"){$fn=[];foreach($Mj
as$u=>$X)$fn[]="$u = $X";$H=table($Q)." SET$Hk".implode(",$Hk",$fn);return
queries("UPDATE".($w?limit1($Q,$H,$Hj,$Hk):" $H$Hj"));}function
insert($Q,array$Mj){return
queries("INSERT INTO ".table($Q).($Mj?" (".implode(", ",array_keys($Mj)).")\nVALUES (".implode(", ",$Mj).")":" DEFAULT VALUES").$this->getInsertReturningSql($Q));}function
getInsertReturningSql($Q){return"";}function
insertUpdate($Q,array$Nj,array$wj){return
false;}function
begin(){remember_query("BEGIN");return$this->connection->begin();}function
commit(){remember_query("COMMIT");return$this->connection->commit();}function
rollback(){remember_query("ROLLBACK");return$this->connection->rollback();}function
slowQuery($H,$jm){return
null;}function
convertSearch($nf,array$Z,array$k){return$nf;}function
isSearchable(array$Z,array$k){return
true;}function
getNull(){return"NULL";}function
getTypeName(stdClass$k){return
isset($k->native_type)?$k->native_type:"";}function
quoteBinary($P){return
q($P);}function
warnings(){return
null;}function
tableHelp($A,$Qf=false){return
null;}function
supportsIndex(array$Jl){return!is_view($Jl);}function
supportsAlterIndex(array$Jl){return
true;}function
getIndexAlgorithms(array$Jl){return[];}function
getIndexOpclasses(){return[];}function
getInheritedTables($Q){return[];}function
getParentTables($Q){return[];}function
isPartition($Q){return
false;}function
getPartitionsInfo($Q){return[];}function
hasCStyleEscapes(){return
false;}function
lineComment(){return"--";}function
engines(){return[];}function
explodeArrayValue($Y,$U,&$qk){return[];}function
implodeArrayValues(array$fn,$U){return"";}function
checkConstraints($Q){return
get_key_vals("SELECT c.CONSTRAINT_NAME, CHECK_CLAUSE
FROM INFORMATION_SCHEMA.CHECK_CONSTRAINTS c
JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS t ON c.CONSTRAINT_SCHEMA = t.CONSTRAINT_SCHEMA
	AND c.CONSTRAINT_NAME = t.CONSTRAINT_NAME".($this->connection->isMariaDB()?" AND c.TABLE_NAME = ".q($Q):"")."
WHERE c.CONSTRAINT_SCHEMA = ".($_GET["ns"]!=""?q($_GET["ns"]):(DIALECT=="pgsql"?"current_schema()":"DATABASE()"))."
AND t.TABLE_NAME = ".q($Q).(DIALECT=="pgsql"?"
AND CHECK_CLAUSE NOT LIKE '% IS NOT NULL'":""),$this->connection);}function
getAllFields(){if(DB=="")return[];$Ba=[];$ag=(DIALECT=="pgsql"||DIALECT=="mssql");$wj=(DIALECT=="sql"?"c.COLUMN_KEY = 'PRI'":($ag?"k.COLUMN_NAME":""));$L=get_rows("SELECT c.TABLE_NAME AS tab, c.COLUMN_NAME AS field, c.IS_NULLABLE AS nullable,
	c.DATA_TYPE AS type, c.CHARACTER_MAXIMUM_LENGTH AS length".($wj?",
	$wj AS ".idf_escape("primary"):"")."
FROM INFORMATION_SCHEMA.COLUMNS c".($ag?"
LEFT JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS t
	ON c.TABLE_SCHEMA = t.TABLE_SCHEMA AND c.TABLE_NAME = t.TABLE_NAME AND t.CONSTRAINT_TYPE = 'PRIMARY KEY'
LEFT JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE k
	ON t.CONSTRAINT_SCHEMA = k.CONSTRAINT_SCHEMA AND t.CONSTRAINT_NAME = k.CONSTRAINT_NAME
		AND c.TABLE_SCHEMA = k.TABLE_SCHEMA AND c.TABLE_NAME = k.TABLE_NAME AND c.COLUMN_NAME = k.COLUMN_NAME":"")."
WHERE c.TABLE_SCHEMA = ".q($_GET["ns"]!=""?$_GET["ns"]:DB)."
ORDER BY c.TABLE_NAME, c.ORDINAL_POSITION",$this->connection);foreach($L
as$K){$K["null"]=($K["nullable"]=="YES");$Ba[$K["tab"]][]=$K;}return$Ba;}}Drivers::add("mysql","MySQL",["MySQLi","PDO_MySQL"]);if(isset($_GET["mysql"])){define("AdminNeo\DRIVER","mysql");define("AdminNeo\DIALECT","sql");if(extension_loaded("mysqli")&&$_GET["ext"]!="pdo"){define("AdminNeo\DRIVER_EXTENSION","MySQLi");class
MySqlConnection
extends
Connection{private$mysqli;protected
function
__construct(){parent::__construct();$this->mysqli=new
mysqli();$this->mysqli->init();}function
getDefaultServerName(){return"localhost";}function
open($N,$V,$F){mysqli_report(MYSQLI_REPORT_OFF);list($gf,$nj)=host_port($N);$u=Admin::get()->getConfig()->getSslKey();$lb=Admin::get()->getConfig()->getSslCertificate();$jb=Admin::get()->getConfig()->getSslCaCertificate();$ll=$u||$lb||$jb;if($ll){$this->mysqli->ssl_set($u,$lb,$jb,null,null);$ke=Admin::get()->getConfig()->getSslTrustServerCertificate()?64:MYSQLI_CLIENT_SSL;}else$ke=0;$Ub=@$this->mysqli->real_connect(($N!=""?$gf:ini_get("mysqli.default_host")),($N.$V!=""?$V:ini_get("mysqli.default_user")),($N.$V.$F!=""?$F:ini_get("mysqli.default_pw")),null,(is_numeric($nj)?(int)$nj:ini_get("mysqli.default_port")),(!is_numeric($nj)?$nj:null),$ke);$this->mysqli->options(MYSQLI_OPT_LOCAL_INFILE,false);if($Ub){$xf=$this->mysqli->get_server_info();$this->version=str_replace("-MariaDB","",$xf);$this->flavor=str_contains($xf,"MariaDB")?"mariadb":null;}return$Ub;}function
getAffectedRows(){return$this->mysqli->affected_rows;}function
getErrno(){return$this->mysqli->errno;}function
getError(){return$this->mysqli->error;}function
selectDatabase($A){return$this->mysqli->select_db($A);}function
setCharset($pb){if($this->mysqli->set_charset($pb))return
true;$this->mysqli->set_charset('utf8');return(bool)$this->query("SET NAMES $pb");}function
quote($P){return"'".$this->mysqli->escape_string($P)."'";}function
query($H,$Fm=false){$I=$this->mysqli->query($H,($Fm?MYSQLI_USE_RESULT:MYSQLI_STORE_RESULT));return
is_object($I)?new
MySqlResult($I):$I;}function
getQueryInfo(){return$this->mysqli->info;}function
getWarningsCount(){return$this->mysqli->warning_count;}function
multiQuery($H){return$this->mysqli->multi_query($H);}function
storeResult($I=null){$I=$this->mysqli->store_result();if(!$I)return
false;return
new
MySqlResult($I);}function
hasMoreResults(){return$this->mysqli->more_results();}function
nextResult(){return$this->mysqli->more_results()&&($this->mysqli->next_result()||$this->mysqli->errno);}}class
MySqlResult
extends
Result{private$resource;function
__construct(mysqli_result$bk){parent::__construct($bk->num_rows);$this->resource=$bk;}function
fetchAssoc(){return$this->resource->fetch_assoc();}function
fetchRow(){return$this->resource->fetch_row();}function
fetchField(){return$this->resource->fetch_field();}function
seek($Xh){return$this->resource->data_seek($Xh);}}}elseif(extension_loaded("pdo_mysql")){define("AdminNeo\DRIVER_EXTENSION","PDO_MySQL");class
MySqlConnection
extends
PdoConnection{function
getDefaultServerName(){return"localhost";}function
open($N,$V,$F){list($gf,$nj)=host_port($N);$fd="mysql:charset=utf8".($gf!=""?";host=$gf":"").($nj?(is_numeric($nj)?";port=":";unix_socket=").$nj:"");$C=[PDO::MYSQL_ATTR_LOCAL_INFILE=>false];if(isset($_GET["select"])&&defined('\PDO::MYSQL_ATTR_MULTI_STATEMENTS'))$C[PDO::MYSQL_ATTR_MULTI_STATEMENTS]=false;$u=Admin::get()->getConfig()->getSslKey();if($u)$C[PDO::MYSQL_ATTR_SSL_KEY]=$u;$lb=Admin::get()->getConfig()->getSslCertificate();if($lb)$C[PDO::MYSQL_ATTR_SSL_CERT]=$lb;$jb=Admin::get()->getConfig()->getSslCaCertificate();if($jb)$C[PDO::MYSQL_ATTR_SSL_CA]=$jb;$_m=Admin::get()->getConfig()->getSslTrustServerCertificate();if($_m!==null&&defined('\PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT'))$C[PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT]=!$_m;if(!$this->dsn($fd,$V,$F,$C))return
false;$jn=@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION);$this->flavor=str_contains($jn,"MariaDB")?"mariadb":null;return
true;}function
setCharset($pb){return(bool)$this->query("SET NAMES $pb");}function
selectDatabase($A){return(bool)$this->query("USE ".idf_escape($A));}function
query($H,$Fm=false){$this->pdo->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY,!$Fm);return
parent::query($H,$Fm);}}}class
MySqlDriver
extends
Driver{protected
function
__construct(Connection$d,$ya){parent::__construct($d,$ya);$this->types=[lang(124)=>["tinyint"=>3,"smallint"=>5,"mediumint"=>8,"int"=>10,"bigint"=>20,"decimal"=>66,"float"=>12,"double"=>21,],lang(125)=>["date"=>10,"datetime"=>19,"timestamp"=>19,"time"=>10,"year"=>4,],lang(126)=>["char"=>255,"varchar"=>65535,"tinytext"=>255,"text"=>65535,"mediumtext"=>16777215,"longtext"=>4294967295,],lang(127)=>["enum"=>65535,"set"=>64,],lang(128)=>["bit"=>20,"binary"=>255,"varbinary"=>65535,"tinyblob"=>255,"blob"=>65535,"mediumblob"=>16777215,"longblob"=>4294967295,],lang(129)=>["geometry"=>0,"point"=>0,"linestring"=>0,"polygon"=>0,"multipoint"=>0,"multilinestring"=>0,"multipolygon"=>0,"geometrycollection"=>0,],];$this->unsigned=["unsigned","zerofill","unsigned zerofill"];$Qg=$d->isMariaDB();if($d->isMinVersion($Qg?"10.2":"5.7"))$this->generated=["STORED","VIRTUAL"];$this->operators=["=","<",">","<=",">=","!=","LIKE","LIKE %%","NOT LIKE","IN","NOT IN","FIND_IN_SET","IS NULL","IS NOT NULL","REGEXP","NOT REGEXP","SQL",];$this->functions=["char_length","lower","upper","round","floor","ceil","date","from_unixtime","unix_timestamp","sec_to_time","time_to_sec",];$this->grouping=["sum","min","max","avg","count","count distinct","group_concat",];$this->partitionBy=["RANGE","LIST","HASH","LINEAR HASH","KEY","LINEAR KEY"];$this->insertFunctions=["char"=>"md5/sha1/password/encrypt/uuid","binary"=>"md5/sha1","date|time"=>"now",];$this->editFunctions=[number_type()=>"+/-","date"=>"+ interval/- interval","time"=>"addtime/subtime","char|text"=>"concat",];if($d->isMinVersion($Qg?"10.2":"5.7.8"))$this->types[lang(126)]["json"]=4294967295;if($Qg&&$d->isMinVersion("10.7")){$this->types[lang(126)]["uuid"]=128;$this->insertFunctions['uuid']='uuid';}if($Qg&&$d->isMinVersion("10.5")){$this->types[lang(130)]["inet6"]=39;if($d->isMinVersion("10.10"))$this->types[lang(130)]["inet4"]=15;}if($d->isMinVersion($Qg?"11.7":"9"))$this->types[lang(124)]["vector"]=16383;$this->systemDatabases=["mysql","information_schema","performance_schema","sys"];}function
insert($Q,array$Mj){return($Mj?parent::insert($Q,$Mj):queries("INSERT INTO ".table($Q)." ()\nVALUES ()"));}function
getUnconvertFunction(array$k){if(preg_match("~binary~",$k["type"]))return"<code class='jush-sql'>UNHEX</code>";elseif($k["type"]=="bit")return
doc_link(['sql'=>'bit-value-literals.html','mariadb'=>"reference/sql-structure/sql-language-structure/binary-literals"],"<code>b''</code>");elseif($k["type"]=="vector")return"<code class='jush-sql'>".($this->connection->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."</code>";elseif(preg_match("~geometry|point|linestring|polygon~",$k["type"]))return"<code class='jush-sql'>GeomFromText</code>";else
return"";}function
getTypeName(stdClass$k){$Em=["decimal","tinyint","smallint","int","float","double",7=>"timestamp","bigint","mediumint","date","time","datetime","year",15=>"varchar","bit",242=>"vector",245=>"json","decimal","enum","set","tinytext","mediumtext","longtext","text","varchar","char","geometry",];$U=isset($Em[$k->type])?$Em[$k->type]:"";return
parent::getTypeName($k)?:($k->charsetnr==63?str_replace(["text","varchar","char"],["blob","varbinary","binary"],$U):$U);}function
quoteBinary($P){return"X".q(bin2hex($P));}function
insertUpdate($Q,array$Nj,array$wj){$c=array_keys(reset($Nj));$tj="INSERT INTO ".table($Q)." (".implode(", ",$c).") VALUES\n";$fn=[];foreach($c
as$u)$fn[$u]="$u = VALUES($u)";$yl="\nON DUPLICATE KEY UPDATE ".implode(", ",$fn);$fn=[];$v=0;foreach($Nj
as$Mj){$Y="(".implode(", ",$Mj).")";if($fn&&(strlen($tj)+$v+strlen($Y)+strlen($yl)>1e6)){if(!queries($tj.implode(",\n",$fn).$yl))return
false;$fn=[];$v=0;}$fn[]=$Y;$v+=strlen($Y)+2;}return
queries($tj.implode(",\n",$fn).$yl);}function
slowQuery($H,$jm){$Qg=$this->connection->isMariaDB();if(!$this->connection->isMinVersion($Qg?"10.1.2":"5.7.8"))return
null;if($Qg)return"SET STATEMENT max_statement_time=$jm FOR $H";elseif(preg_match('~^(SELECT\b)(.+)~is',$H,$z))return"$z[1] /*+ MAX_EXECUTION_TIME(".($jm*1000).") */ $z[2]";else
return
null;}function
convertSearch($nf,array$Z,array$k){return(preg_match('~char|text|enum|set~',$k["type"])&&!preg_match("~^utf8~",$k["collation"])&&preg_match('~[\x80-\xFF]~',$Z['val'])?"CONVERT($nf USING ".charset($this->connection).")":$nf);}function
isSearchable(array$Z,array$k){return$k["type"]!="bit"||preg_match('~IN$|NULL$|^(FIND_IN_SET|SQL)$~',$Z["op"])||($Z["op"]!="LIKE %%"&&preg_match('~^[01]+$~',$Z["val"]));}function
warnings(){$I=$this->connection->query("SHOW WARNINGS");if($I&&$I->getRowsCount()){ob_start();print_select_result($I);return
ob_get_clean();}return
null;}function
tableHelp($A,$Qf=false){$Qg=$this->connection->isMariaDB();if(DB=="information_schema"){$A=strtolower($A);return$Qg?"reference/system-tables/information-schema/information-schema-tables/".(str_starts_with($A,"innodb_")?"information-schema-innodb-tables/":"")."information-schema-$A-table":"information-schema-".str_replace("_","-",$A)."-table.html";}if(DB=="performance_schema")return$Qg?"reference/system-tables/performance-schema/performance-schema-tables/performance-schema-$A-table":"performance-schema-".str_replace("_","-",$A)."-table.html";if(DB=="sys"){if($Qg)return"reference/system-tables/sys-schema/";return"sys-".strtolower(str_replace("_","-",preg_replace('~^x\$~','',$A))).".html";}if(DB=="mysql")return$Qg?"reference/system-tables/the-mysql-database-tables/mysql-$A".str_starts_with($A,"innodb_")?"":"-table":"system-schema.html";return
null;}function
getPartitionsInfo($Q){$ze="FROM information_schema.PARTITIONS WHERE TABLE_SCHEMA = ".q(DB)." AND TABLE_NAME = ".q($Q);$I=Connection::get()->query("SELECT PARTITION_METHOD, PARTITION_EXPRESSION, PARTITION_ORDINAL_POSITION $ze ORDER BY PARTITION_ORDINAL_POSITION DESC LIMIT 1")->fetchRow();if(!$I)return[];$xf=["partition_by"=>$I[0],"partition"=>$I[1],"partitions"=>$I[2],];$Wi=get_key_vals("SELECT PARTITION_NAME, PARTITION_DESCRIPTION $ze AND PARTITION_NAME != '' ORDER BY PARTITION_ORDINAL_POSITION");$xf["partition_names"]=array_keys($Wi);$xf["partition_values"]=array_values($Wi);return$xf;}function
getIndexAlgorithms(array$Jl){return
preg_match('~^(MEMORY|NDB)$~',$Jl["Engine"])?["BTREE","HASH"]:["BTREE"];}function
hasCStyleEscapes(){static$hb;if($hb===null){$kl=$this->connection->getValue("SHOW VARIABLES LIKE 'sql_mode'",1);$hb=(strpos($kl,'NO_BACKSLASH_ESCAPES')===false);}return$hb;}function
lineComment(){return"#|-- ";}function
engines(){$wd=[];foreach(get_rows("SHOW ENGINES")as$K){if(preg_match("~YES|DEFAULT~",$K["Support"]))$wd[]=$K["Engine"];}return$wd;}}function
create_driver(Connection$d){return
MySqlDriver::create($d,Admin::get());}function
idf_escape($nf){return"`".str_replace("`","``",$nf)."`";}function
table($nf){return
idf_escape($nf);}function
connect($wj=false,&$j=null){$d=$wj?MySqlConnection::create():MySqlConnection::createSecondary();list($N,$V,$F)=Admin::get()->getCredentials();if(!$d->openPasswordless($N,$V,$F,false)){$j=$d->getError();if(function_exists('iconv')&&!is_utf8($j)&&strlen($mk=iconv("windows-1252","utf-8//IGNORE",$j))>strlen($j))$j=$mk;return
null;}$d->setCharset(charset($d));$d->query("SET sql_quote_show_create = 1, autocommit = 1");if($wj&&$d->isMariaDB()){Drivers::setName(DRIVER,"MariaDB");save_driver_name(DRIVER,$N,"MariaDB");}return$d;}function
get_databases($me){$g=get_session("dbs");if($g===null){$H="SELECT SCHEMA_NAME FROM information_schema.SCHEMATA ORDER BY SCHEMA_NAME";$ml=microtime(true);$g=($me?slow_query($H):get_vals($H));if(microtime(true)-$ml>0.1){restart_session();set_session("dbs",$g);stop_session();}}return$g;}function
limit($H,$Z,$w,$Xh=0,$Hk=" "){return" $H$Z".($w?$Hk."LIMIT $w".($Xh?" OFFSET $Xh":""):"");}function
limit1($Q,$H,$Z,$Hk="\n"){return
limit($H,$Z,1,0,$Hk);}function
db_collation($h,array$Db){$J=null;$e=Connection::get()->getValue("SHOW CREATE DATABASE ".idf_escape($h),1);if(preg_match('~ COLLATE ([^ ]+)~',$e,$z))$J=$z[1];elseif(preg_match('~ CHARACTER SET ([^ ]+)~',$e,$z))$J=$Db[$z[1]][-1];return$J;}function
logged_user(){return
Connection::get()->getValue("SELECT USER()");}function
tables_list(){return
get_key_vals("SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() ORDER BY TABLE_NAME");}function
count_tables(array$g){$J=[];foreach($g
as$h)$J[$h]=count(get_vals("SHOW TABLES IN ".idf_escape($h)));return$J;}function
table_status($A="",$Ud=false){if($Ud)$H="SELECT TABLE_NAME AS Name, ENGINE AS Engine, CREATE_OPTIONS AS Create_options,
	TABLES.TABLE_COLLATION AS Collation, TABLE_COMMENT AS Comment
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() ".($A!=""?"AND TABLE_NAME = ".q($A):"ORDER BY Name");else$H="SHOW TABLE STATUS".($A!=""?" LIKE ".q(addcslashes($A,"%_\\")):"");$S=[];foreach(get_rows($H)as$K){if($K["Engine"]=="InnoDB")$K["Comment"]=preg_replace('~(?:(.+); )?InnoDB free: .*~','\1',$K["Comment"]);if(!isset($K["Engine"]))$K["Comment"]="";if($A!="")$K["Name"]=$A;$S[$K["Name"]]=$K;}return$S;}function
is_view(array$R){return$R["Engine"]===null;}function
fk_support(array$R){return
preg_match('~InnoDB|IBMDB2I'.(Connection::get()->isMinVersion("5.6")?'|NDB':'').'~i',$R["Engine"]);}function
parse_type($Ae){preg_match('~^([^( ]+)(?:\((.+)\))?( unsigned)?( zerofill)?$~',$Ae,$_);return[$_[1],isset($_[2])?$_[2]:"",ltrim((isset($_[3])?$_[3]:"").(isset($_[4])?$_[4]:""))];}function
fields($Q){$Qg=Connection::get()->isMariaDB();$J=[];foreach(get_rows("SELECT * FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ".q($Q)." ORDER BY ORDINAL_POSITION")as$K){$k=$K["COLUMN_NAME"];$U=preg_replace('~\s?/\*.+\*/~U',"",$K["COLUMN_TYPE"]);$Qd=$K["EXTRA"];preg_match('~^(VIRTUAL|PERSISTENT|STORED)~',$Qd,$De);list($Dm,$v,$Nm)=parse_type($U);$i=$Qg&&$K["COLUMN_DEFAULT"]=="NULL"?null:$K["COLUMN_DEFAULT"];if($i!==null){$Vf=preg_match('~(text|json)~',$Dm);if(!$Qg&&$Vf)$i=preg_replace("~^(_\w+)?('.*')$~",'\2',stripslashes($i));if($Qg||$Vf){$i=preg_replace_callback("~^'(.*)'$~",function($_){return
stripslashes(str_replace("''","'",$_[1]));},$i);}if(!$Qg&&preg_match('~binary~',$Dm)&&preg_match('~^0x(\w*)$~',$i,$_))$i=pack("H*",$_[1]);}$Fe=$K["GENERATION_EXPRESSION"];if(!$Qg)$Fe=preg_replace("~(^|,|\()(_\w+)?('.*')($|,|\))~",'\1\3\4',stripslashes($Fe));$J[$k]=["field"=>$k,"full_type"=>$U,"type"=>$Dm,"length"=>$v,"unsigned"=>$Nm,"default"=>($De?$Fe:$i),"null"=>($K["IS_NULLABLE"]=="YES"),"auto_increment"=>($Qd=="auto_increment"),"on_update"=>(preg_match('~\bon update (\w+)~i',$Qd,$_)?$_[1]:""),"collation"=>$K["COLLATION_NAME"],"privileges"=>array_flip(explode(",",$K["PRIVILEGES"]))+["where"=>1,"order"=>1],"comment"=>$K["COLUMN_COMMENT"],"primary"=>($K["COLUMN_KEY"]=="PRI"),"generated"=>($De[1]=="PERSISTENT"?"STORED":$De[1]),];}return$J;}function
indexes($Q,$d=null){$J=[];foreach(get_rows("SHOW INDEX FROM ".table($Q),$d)as$K){$A=$K["Key_name"];$J[$A]["type"]=($A=="PRIMARY"?"PRIMARY":($K["Index_type"]=="FULLTEXT"?"FULLTEXT":($K["Non_unique"]?(preg_match('~^(SPATIAL|VECTOR)$~',$K["Index_type"])?$K["Index_type"]:"INDEX"):"UNIQUE")));$J[$A]["columns"][]=$K["Column_name"];$J[$A]["lengths"][]=($K["Index_type"]=="SPATIAL"?null:$K["Sub_part"]);$J[$A]["descs"][]=($K["Collation"]=="D"?'1':null);$J[$A]["algorithm"]=$K["Index_type"];}return$J;}function
foreign_keys($Q){static$cj='(?:`(?:[^`]|``)+`|"(?:[^"]|"")+")';$J=[];$ic=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if($ic){$hi=implode("|",Driver::get()->getOnActions());preg_match_all("~CONSTRAINT ($cj) FOREIGN KEY ?\\(((?:$cj,? ?)+)\\) REFERENCES ($cj)(?:\\.($cj))? "."\\(((?:$cj,? ?)+)\\)(?: ON DELETE ($hi))?(?: ON UPDATE ($hi))?~",$ic,$_,PREG_SET_ORDER);foreach($_
as$z){preg_match_all("~$cj~",$z[2],$el);preg_match_all("~$cj~",$z[5],$Yl);$J[idf_unescape($z[1])]=["db"=>idf_unescape($z[4]!=""?$z[3]:$z[4]),"table"=>idf_unescape($z[4]!=""?$z[4]:$z[3]),"source"=>array_map('AdminNeo\idf_unescape',$el[0]),"target"=>array_map('AdminNeo\idf_unescape',$Yl[0]),"on_delete"=>($z[6]?:"RESTRICT"),"on_update"=>($z[7]?:"RESTRICT"),];}}return$J;}function
backward_keys($Q){$H="SELECT CONSTRAINT_NAME AS constraint_name, TABLE_SCHEMA AS table_schema, TABLE_NAME AS table_name,
COLUMN_NAME AS column_name, REFERENCED_COLUMN_NAME AS referenced_column_name
FROM information_schema.KEY_COLUMN_USAGE
WHERE TABLE_SCHEMA = ".q(Admin::get()->getDatabase())."
AND REFERENCED_TABLE_SCHEMA = ".q(Admin::get()->getDatabase())."
AND REFERENCED_TABLE_NAME = ".q($Q)."
ORDER BY ORDINAL_POSITION";return
get_rows($H,null,"");}function
view($A){$M=Connection::get()->getValue("SHOW CREATE VIEW ".table($A),1);$Ng='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$M=preg_replace("~^$Ng\\s+AS\\s+~isU","",$M);return["select"=>format_sql($M)];}function
collations(){$J=[];$H=Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("10.10")?"SELECT CHARACTER_SET_NAME AS Charset, FULL_COLLATION_NAME AS Collation, IS_DEFAULT AS `Default` FROM information_schema.COLLATION_CHARACTER_SET_APPLICABILITY":"SHOW COLLATION";foreach(get_rows($H)as$K){if($K["Default"])$J[$K["Charset"]][-1]=$K["Collation"];else$J[$K["Charset"]][]=$K["Collation"];}ksort($J);foreach($J
as$u=>$X)sort($J[$u]);return$J;}function
information_schema($h,$uk=""){return($h=="information_schema")||(Connection::get()->isMinVersion("5.5")&&$h=="performance_schema");}function
error(){return
h(preg_replace('~^You have an error.*syntax to use~U',"Syntax error",Connection::get()->getError()));}function
create_database($h,$Cb){return(bool)queries("CREATE DATABASE ".idf_escape($h).($Cb?" COLLATE ".q($Cb):""));}function
drop_databases(array$g){$J=apply_queries("DROP DATABASE",$g,'AdminNeo\idf_escape');restart_session();set_session("dbs",null);return$J;}function
rename_database($A,$Cb){$J=false;if(create_database($A,$Cb)){$S=[];$mn=[];foreach(tables_list()as$Q=>$U){if($U=='VIEW')$mn[]=$Q;else$S[]=$Q;}$J=(!$S&&!$mn)||move_tables($S,$mn,$A);drop_databases($J?[DB]:[]);}return$J;}function
auto_increment(){$Pa=" PRIMARY KEY";if($_GET["create"]!=""&&$_POST["auto_increment_col"]){foreach(indexes($_GET["create"])as$s){if(in_array($_POST["fields"][$_POST["auto_increment_col"]]["orig"],$s["columns"],true)){$Pa="";break;}if($s["type"]=="PRIMARY")$Pa=" UNIQUE";}}return" AUTO_INCREMENT$Pa";}function
alter_table($Q,$A,array$l,array$oe,$Mb,$vd,$Cb,$Oa,$Vi){$Ga=[];foreach($l
as$k){if($k[1]){$i=$k[1][3];if(str_contains($i," GENERATED")){$k[1][3]=Connection::get()->isMariaDB()?"":$k[1][2];$k[1][2]=$i;}$Ga[]=($Q!=""?($k[0]!=""?"CHANGE ".idf_escape($k[0]):"ADD"):" ")." ".implode($k[1]).($Q!=""?$k[2]:"");}else$Ga[]="DROP ".idf_escape($k[0]);}$Ga=array_merge($Ga,$oe);$ol=($Mb!==null?" COMMENT=".q($Mb):"").($vd?" ENGINE=".q($vd):"").($Cb?" COLLATE ".q($Cb):"").($Oa!=""?" AUTO_INCREMENT=$Oa":"");if($Vi){$Wi=[];if($Vi["partition_by"]=='RANGE'||$Vi["partition_by"]=='LIST'){foreach($Vi["partition_names"]as$u=>$X){$Y=$Vi["partition_values"][$u];$Wi[]="\n  PARTITION ".idf_escape($X)." VALUES ".($Vi["partition_by"]=='RANGE'?"LESS THAN":"IN").($Y!=""?" ($Y)":" MAXVALUE");}}$ol
.="\nPARTITION BY {$Vi["partition_by"]}({$Vi["partition"]})";if($Wi)$ol
.=" (".implode(",",$Wi)."\n)";elseif($Vi["partitions"])$ol
.=" PARTITIONS ".(int)$Vi["partitions"];}elseif($Vi===null)$ol
.="\nREMOVE PARTITIONING";if($Q=="")return(bool)queries("CREATE TABLE ".table($A)." (\n".implode(",\n",$Ga)."\n)$ol");if($Q!=$A)$Ga[]="RENAME TO ".table($A);if($ol)$Ga[]=ltrim($ol);return!$Ga||queries("ALTER TABLE ".table($Q)."\n".implode(",\n",$Ga));}function
alter_indexes($Q,array$Ga){$nb=[];foreach($Ga
as$u=>$X)$nb[]=($X[2]=="DROP"?"\nDROP INDEX ".idf_escape($X[1]):"\nADD $X[0] ".($X[0]=="PRIMARY"?"KEY ":"").($X[1]!=""?idf_escape($X[1])." ":"")."(".implode(", ",$X[2]).")");return(bool)queries("ALTER TABLE ".table($Q).implode(",",$nb));}function
truncate_tables(array$S){return
apply_queries("TRUNCATE TABLE",$S);}function
drop_views(array$mn){return(bool)queries("DROP VIEW ".implode(", ",array_map('AdminNeo\table',$mn)));}function
drop_tables(array$S){return(bool)queries("DROP TABLE ".implode(", ",array_map('AdminNeo\table',$S)));}function
move_tables(array$S,array$mn,$Yl){$Yj=[];foreach($S
as$Q)$Yj[]=table($Q)." TO ".idf_escape($Yl).".".table($Q);if(!$Yj||queries("RENAME TABLE ".implode(", ",$Yj))){$Dc=[];foreach($mn
as$Q)$Dc[table($Q)]=view($Q);Connection::get()->selectDatabase($Yl);$h=idf_escape(DB);foreach($Dc
as$A=>$kn){if(!queries("CREATE VIEW $A AS ".str_replace(" $h."," ",$kn["select"]))||!queries("DROP VIEW $h.$A"))return
false;}return
true;}return
false;}function
copy_tables(array$S,array$mn,$Yl){queries("SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO'");foreach($S
as$Q){$A=($Yl==DB?table("copy_$Q"):idf_escape($Yl).".".table($Q));if(($_POST["overwrite"]&&!queries("\nDROP TABLE IF EXISTS $A"))||!queries("CREATE TABLE $A LIKE ".table($Q))||!queries("INSERT INTO $A SELECT * FROM ".table($Q)))return
false;foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")))as$K){$xm=$K["Trigger"];if(!queries("CREATE TRIGGER ".($Yl==DB?idf_escape("copy_$xm"):idf_escape($Yl).".".idf_escape($xm))." $K[Timing] $K[Event] ON $A FOR EACH ROW\n$K[Statement];"))return
false;}}foreach($mn
as$Q){$A=($Yl==DB?table("copy_$Q"):idf_escape($Yl).".".table($Q));$kn=view($Q);if(($_POST["overwrite"]&&!queries("DROP VIEW IF EXISTS $A"))||!queries("CREATE VIEW $A AS $kn[select]"))return
false;}return
true;}function
trigger($A,$Q){if($A=="")return[];$L=get_rows("SHOW TRIGGERS WHERE `Trigger` = ".q($A));return
reset($L);}function
triggers($Q){$J=[];foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")))as$K)$J[$K["Trigger"]]=[$K["Timing"],$K["Event"]];return$J;}function
trigger_options(){return["Timing"=>["BEFORE","AFTER"],"Event"=>["INSERT","UPDATE","DELETE"],"Type"=>["FOR EACH ROW"],];}function
routine($A,$U){if($A=="")return[];$L=get_rows("SELECT PARAMETER_NAME, DTD_IDENTIFIER, PARAMETER_MODE, CHARACTER_SET_NAME
FROM information_schema.PARAMETERS
WHERE SPECIFIC_SCHEMA = DATABASE() AND ROUTINE_TYPE = '$U' AND SPECIFIC_NAME = ".q($A)."
ORDER BY ORDINAL_POSITION");$l=[];foreach($L
as$K){$Ae=$K["DTD_IDENTIFIER"];list($Dm,$v,$Nm)=parse_type($Ae);$l[]=["field"=>$K["PARAMETER_NAME"],"type"=>$Dm,"length"=>$v,"unsigned"=>$Nm,"null"=>true,"full_type"=>$Ae,"inout"=>($U=="FUNCTION"?"":$K["PARAMETER_MODE"]),"collation"=>$K["CHARACTER_SET_NAME"],];}$J=Connection::get()->query("SELECT
	ROUTINE_COMMENT comment,
	CONCAT(IF(IS_DETERMINISTIC = 'YES', 'DETERMINISTIC\\n', ''), IF(SQL_DATA_ACCESS != 'CONTAINS SQL', CONCAT(SQL_DATA_ACCESS, '\\n'), '')) characteristics,
	ROUTINE_DEFINITION definition,
	'SQL' language
FROM information_schema.ROUTINES
WHERE ROUTINE_SCHEMA = DATABASE() AND ROUTINE_TYPE = '$U' AND ROUTINE_NAME = ".q($A))->fetchAssoc();$e=Connection::get()->getValue("SHOW CREATE $U ".idf_escape($A),2);$P="'(?:[^'\\\\]|\\\\.)*'";$qm='`[^`]*`|"(?:[^"\\\]|\\\.)*"|$string|/\*.*?\*/|/';$ob="    (?:[A-Z ]+|COMMENT (?:$P)+)\\n";if($e&&preg_match("~^(?:[^(`\"'/]++|$qm)*?(\\((?:[^()`\"'/]++|$qm|(?1))*+\\))(?: RETURNS [^\\n]*)?\\n(?:$ob)*~s",$e,$z))$J['definition']=substr($e,strlen($z[0]));$J['definition']=$J['characteristics'].$J['definition'];unset($J['characteristics']);if($l&&$l[0]['field']=='')$J['returns']=array_shift($l);$J['fields']=$l;return$J;}function
routines(){return
get_rows("SELECT SPECIFIC_NAME, ROUTINE_NAME, ROUTINE_TYPE, DTD_IDENTIFIER, ROUTINE_COMMENT FROM information_schema.ROUTINES WHERE ROUTINE_SCHEMA = DATABASE()");}function
routine_languages(){return[];}function
routine_id($A,array$K){return
idf_escape($A);}function
last_id($I){return
Connection::get()->getValue("SELECT LAST_INSERT_ID()");}function
explain(Connection$d,$H){return$d->query("EXPLAIN ".(Connection::get()->isMinVersion("5.7")?"":"PARTITIONS ").$H);}function
found_rows(array$R,array$Z){return$R["Engine"]=="InnoDB"&&!$Z?(int)$R["Rows"]:null;}function
format_sql($H){$Ng='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$ig='FROM|WHERE|HAVING|GROUP\s+BY|ORDER\s+BY|(NATURAL\s+)?((LEFT|RIGHT)\s+)?((INNER|OUTER|CROSS)\s+)?JOIN';$H=preg_replace("~($Ng)\\s+(AS\\s+SELECT)~isU","$1 AS\nSELECT",$H);$H=preg_replace("~($Ng)\\s+($ig)~isU","$1\n$2",$H);$H=preg_replace("~($Ng),~isU","$1,\n  ",$H);return$H;}function
create_sql($Q,$Oa,$vl){$H=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if(!$Oa)$H=preg_replace('~ AUTO_INCREMENT=\d+~','',$H);return!str_contains($H,"\n")?format_sql($H):$H;}function
truncate_sql($Q){return"TRUNCATE ".table($Q);}function
create_database_sql($tc,$vl=""){$A=idf_escape($tc);$Kb="";if(str_contains($vl,"CREATE")&&($e=Connection::get()->getValue("SHOW CREATE DATABASE $A",1))){set_utf8mb4($e);if($vl=="DROP+CREATE")$Kb="DROP DATABASE IF EXISTS $A;\n";$Kb
.="$e;\n";}return$Kb;}function
use_sql($tc,$vl=""){return"USE ".idf_escape($tc).";\n";}function
trigger_sql($Q){$hl="";foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")),null,"-- ")as$K)$hl
.="\nCREATE TRIGGER ".idf_escape($K["Trigger"])." $K[Timing] $K[Event] ON ".table($K["Table"])." FOR EACH ROW\n$K[Statement];;\n";return$hl;}function
show_variables(){return
get_rows("SHOW VARIABLES");}function
show_status(){return
get_rows("SHOW STATUS");}function
process_list(){return
get_rows("SHOW FULL PROCESSLIST");}function
convert_field(array$k){if(preg_match("~binary~",$k["type"]))return"HEX(".idf_escape($k["field"]).")";if($k["type"]=="bit")return"BIN(".idf_escape($k["field"])." + 0)";if($k["type"]=="vector")return(Connection::get()->isMariaDB()?"VEC_ToText":"VECTOR_TO_STRING")."(".idf_escape($k["field"]).")";if(preg_match("~geometry|point|linestring|polygon~",$k["type"]))return(Connection::get()->isMinVersion("8")?"ST_":"")."AsWKT(".idf_escape($k["field"]).")";return
null;}function
unconvert_field(array$k,$J){if(preg_match("~binary~",$k["type"]))$J="UNHEX($J)";if($k["type"]=="bit")$J="CONVERT(b$J, UNSIGNED)";if($k["type"]=="vector")$J=(Connection::get()->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."($J)";if(preg_match("~geometry|point|linestring|polygon~",$k["type"])){$tj=(Connection::get()->isMinVersion("8")?"ST_":"");$J=$tj."GeomFromText($J, $tj"."SRID($k[field]))";}return$J;}function
support($Vd){return
preg_match('~^(comment|columns|copy|database|drop_col|dump|event|indexes|kill|privileges|move_col|procedure|processlist|routine|sql|status|table|trigger|variables|view'.(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.8.1":"8")?'|descidx':'').(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.2.1":"8.0.16")?'|check':'').(!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8")?'|fast_status':'').')$~',$Vd);}function
kill_process($X){return
queries("KILL ".number($X));}function
connection_id(){return"SELECT CONNECTION_ID()";}function
max_connections(){return(int)Connection::get()->getValue("SELECT @@max_connections");}}$lj="adminneo-plugins";if(is_dir($lj)){foreach(glob("$lj/*.php")as$n)include_once$n;}function
get_compressed_translations($qg){switch($qg){case'en':return'%X/&cbomt,|?z"**gW&<fbys!&IOEIP1geigv6#(#C3vCp^f]TGDYB[K"59W@k`x-Xw.Kn1lh0Q&t#Qb%IGnUgw$3u36/vM_*
<%SMQF=N<O$lz5jRynoNp6._7`CFqasK1jGxLF(Rg5%/KKgvic"&W=+qRf>jgB`bX]l_OiwUq/N#B=][3f!EYYl[MZwvPr,z)O*JDT
H+1A
mj*m^)jg/eK5i2L]]`GC!iUsR]]B+Qdn?MrBOfHG}jd5voufj_p2KMkscL$mYt@JUanvACnXk5a22(8S3r,&5ygUG2y$:GcQEWCKRDDFa43M
rvYZ6DYfHoAMk}fLwokHQ*E#CCsc(ID~u,3r;|Y8FqUe_lEX+TVU[;kcRwR?%mi;]pf0^H0N6*-$Uoe,Ag`I.[=}ZId,ohW|*gkRe1>ycXK3eUE$cTZJ
&5CD#?zG><P&Hkif:wWk!/2_c6MD&"0ED97ko%JS"KmLQqy>=@S$.@*,iR#p_F{$Fdp*jY
aAn04MhRt(vn[B4~w$#$v4cQdpyrs/9`s~7PK_^Ij.eg:UGjH|E/=Mol>V?Ap5[fE4iJq.L#Bg4}?48OaF<6H!3w?c/;`Zk7RqGp%tjX`2w`h(,Chm#*hCnxO^HL2tT46Ua5+DMs/>C@_WSF]Vku/PDz7rr%j1L;TDybG9kqt%do8zWi?X6O%nYn;V!KK:nJ<cfQA/sK0EP-u+y``to=:0R`3"jk?6I%r~lW>AXVbwO2jU1_;uB}+7Mql0A"Zrr2B@kigg*5W9Oa_7?yk3sGX:]G>dhVMTL<=8=FK_tK;Qe9!:41
/7tLlmUOC7$;YVZ-28LmD[<A?l6f321KLbN/OIkyyo/O+K#L,9?E2.Kxoler?uCRfr~x
d!77sd+BEBhsgXdYgjK~I?KA:py#0Vbxv++GM(ZLe]q31`FTVM4^#eROD^P@Bu@L-~q94CD>l`-`pL9/db[{LD$ZQ^e<KMZxn-?jHn%c="n1Zs7l_sIeiC^GmbHmhCeVQe*W6_83;LC@vcWKs&f9lsn$LJA60lKet~kEcJJ)FEeZ?(x=)q/cPt)y*)N-I@sZBO=_(<E`=>f@(za;XDisuq_2XsU
;py@7H>~QFhbuR"4^l?{)CeW:
K,E:aTR:D#kfrF57O4hf3//<v[t6z)o
EjxXqfyzs`f=x98wR{^VLf#J;6VCvaa,5(($Jr2]NR!npClH,_xgr|nG`j*5]6SF@,k:su]W*D!-aoY*v1Ha<81hiu
,Ceh|)Z3E3vX+("z)qZnsT+*RFp!0NVy7
]A0)G#Y+O7DOd!QY16k@|koQL8&-;VJb_!UvS`,FSryKQfd;49G.sol[^iHPq]jePWhl%+`up(@$ovd=L;W5|"l;/feif-KE6,l%JrPP!2ZD|MA4[Vm23ZS;-E5cQ0Bt!0oo(dOQ:``i_5b3*$.YDAx[=]fOW_}r)$p)^Q`vzR:5rlb(5r^./E"KbVBH0^6:WX@uiHo2A&;;21!/MJnN}1*%DL40T:SA{6e7puA@hx>=%J|a420h%)-y+F2e*/qU<gC86kgCF@Q:yYrG0dOd;X"c9BB=cpCLT:$5gA:@*bq#Y+G`RA+.S/yx6#AYa=DTk
FA2-ODk)@">#,XXdN
S]mNFp%/;&KF@b]aryauYB*ZdGAV~,XK~:uW]wbK)HVm3H{+31WBGqOn<U5PrIdT+#+yZ^EZXp<UA%vuljxw"[
$UpMozyD:Mx12&bAFiCW)cSBDbA}i4vWN.fbGTHhG4#-TF(|BDo^F(]*4b@fWJDlQZC8MX=cAk"^*1W"@db7SMe{G!](k
TRiL/6j+JEt|)s`KiUOof~8yd9+^it0a;&^prp[rrW0$vEJdR&Qd+&,&/.u`s";18!d)oHBVdVurDB,!O-^XH:Kw@Enf2b,/?<-C9Xg5MK^3-xtgcOkRjk#Mco8g]/.qqNV^b8v0dG0
NwgX/y#QemuW.yPpaf1F"mtZ)M`AqNOyyzPPotw/g{^.^80wbHaS;tOQjI_aC0#;-C.xpVvT[78iX"vVlgg=xUI#^ucaw56f3FC^-ylWR:_BR(4:O-gkWQI)J>?WF4x/Or$sYO^"]MmwZ7#Jv.s
(r`;2H3qf<`G3LM0==9HF<a8BZYB"&2tZ:p8&/bghHS)>/iL7=8,/{
k0DcBL51xrXacQ
?4".1-5z><A@DrBTUvk>q->>%)tnG7q^kr?G4c_H8WF|!87`Wg@xo$=R)*2u<G*~6P=?["yRX)aHX5D(/:t*v>(Rs%$82*yINID+h},g$zHqaMy~(dt[
fnT7%bqg(GM]Lf}]
3DFeY<*=G{7#3}Tr*I$K($q1+pr4i7/U!eeKG5op(>V+DMWzisS8J!qvT$XW,TWs!p03/;koZ,QG8eb_45UE=R6Qt96eiwR>V|<JPPydv/Cv-Wgh1]Y@c;5{$ys83~!|=6Id!L6bv-:MY<oiX_cIrJ!oIIa9>><qfn0bcoO8z"xOw8ST;O&yhdOGS{(wv77U(L>6:3Se]6R}1&3f41_?/
]S(>vn+y?$T5vn8+TmA.^4>$S>q$VW/;gKu|[uS2iaRRJ!#r70@j$<G|7kTMBB-DWht?r@CY?rJ{-du-3Qd;%G70*:KooP5YDV?rI`kgA(VeIH5M[kWTt^dL-S.q
P9nP_+kV_24A?Ip={S^X^#bhSL[w:u@yP9)V[.mFyKy#jD~1x4g<RmX-.:l@*IhPX
tS;I-
>q"3&sqwd%TmWrG#{=BKVPRAZM!3
@umT6MHwLw2n/B1{5JYufP_Ue7U(:2%~?|Jf+;]V5s^(s$&:faGDTr0/Jzr-1p7?frc$iL(JNI%@uy^V<$kzg5Q]T]CflA6v`7:thavI+>0S#aA:f4pqw|:$mO,KdN04-#9fGvtSsKX,<E[MvYDt-gfT]Af&wF)?e,,uMb=sOpixV;Txla+x*lh|K$K/TA7g-WwHaQTa.-:)TmZ^/v[SPinyacQK6%O6EO;9eLug]:aP%Zu;?qv8:f0Y#9J`<P>[E[=b>[A^EBf}GXt}5lltvhNq7DK_v2:31*cw?2V|o{WMf<-<Oa?/k2=n+rphh:<th0!b?5BYg{)*OgWvx;4gb?BnmBd#AR<cqzqQJ0MxE[>35g:5xA?UN>/cLYEZ9"pti<IIDqr&#u5o6hvMWOs@,dUp(UuI;8fm^N';}}function
get_translations($qg){$Kc=$qg!="en"?decompress_string(get_compressed_translations("en")):"";return
json_decode(decompress_string(get_compressed_translations($qg),$Kc),true);}function
get_plural_translation_id($u){$mj=array('Too many unsuccessful logins, try again in %d minute(s).'=>142,'%d process(es) have been killed.'=>281,'%d query(s) executed OK.'=>199,'Query executed OK, %d row(s) affected.'=>197,'%d row(s) have been imported.'=>288,'Routine has been called, %d row(s) affected.'=>233,'%d row(s)'=>196,'%d byte(s)'=>43,'%d item(s) have been affected.'=>285,);return
isset($mj[$u])?$mj[$u]:null;}$wm=$_SESSION["translations"];$rg=Locale::get()->getLanguage();if($_SESSION["translations_version"]!=1904488331){$wm=[];$_SESSION["translations_version"]=1904488331;}if($_SESSION["translations_language"]!=$rg){$wm=[];$_SESSION["translations_language"]=$rg;}if(!$wm){$wm=get_translations($rg);$_SESSION["translations"]=$wm;}Locale::get()->setTranslations($wm);$ya=null;$nc=false;$Gf=null;if(function_exists('\adminneo_instance')){$ya=\adminneo_instance();$nc=true;}elseif(file_exists("adminneo-instance.php")){$ya=include_once"adminneo-instance.php";$nc=true;}if($nc&&!$ya
instanceof
Admin&&!$ya
instanceof
Pluginer){$ya=null;$Gg="href=https://github.com/adminneo-org/adminneo#advanced-customizations ".target_blank();$Gf=lang(131,"<b>adminneo-instance.php</b>","<b>adminneo_instance()</b>","Admin::create()")." <a $Gg>".lang(1)."</a>";}if(!$ya)$ya=Admin::create();if($Gf)$ya->addError($Gf);if($sj!==null&&!isset($_GET["settings"])){$ya->getSettings()->updateParameter("lang",$sj);redirect(remove_from_uri());}if(!defined("AdminNeo\DRIVER")){define("AdminNeo\DRIVER",null);define("AdminNeo\DIALECT",null);}define("AdminNeo\SERVER",DRIVER?$_GET[DRIVER]:null);define("AdminNeo\DB",isset($_GET["db"])?$_GET["db"]:"");define("AdminNeo\BASE_URL",preg_replace('~\?.*~','',relative_uri()));define("AdminNeo\ME",BASE_URL.'?'.(sid()?session_name()."=".urlencode(session_id()).'&':'').($_GET["ext"]?"ext=".urlencode($_GET["ext"]).'&':'').(SERVER!==null?DRIVER."=".urlencode(SERVER).'&':'').(isset($_GET["username"])?"username=".urlencode($_GET["username"]).'&':'').(DB!=""?'db='.urlencode(DB).'&'.(isset($_GET["ns"])?"ns=".urlencode($_GET["ns"])."&":""):''));define("AdminNeo\HOME_URL",BASE_URL?:".");define("AdminNeo\SERVER_HOME_URL",substr(preg_replace('~\b(username|db|ns)=[^&]*&~','',ME),0,-1)?:".");if(isset($_GET["set"])){header("Content-Type: text/javascript; charset=utf-8");if(!verify_token()){header("HTTP/1.1 403 Forbidden");exit;}if($_GET["set"]=="navigation-width"){$_n=isset($_POST["width"])?$_POST["width"]:"";if($_n!=""){$_n=min(max((float)$_n,Settings::$NavigationWidthMin),Settings::$NavigationWidthMax);Admin::get()->getSettings()->updateParameter("navigationWidth",sprintf("%.2F",$_n));}else
Admin::get()->getSettings()->updateParameter("navigationWidth",null);}if($_GET["set"]=="export-settings")Admin::get()->getSettings()->updateParameters(["exportFormat"=>isset($_POST["format"])?$_POST["format"]:"","exportOutput"=>isset($_POST["output"])?$_POST["output"]:"",]);exit;}const
VERSION="5.9.0";function
page_header($T,$db=[]){if(!headers_sent()&&!array_sum(array_column(ob_get_status(true),"buffer_used")))ini_set("zlib.output_compression","1");page_headers();if(is_ajax()&&Admin::get()->getErrors()){page_messages();exit;}if(!ob_get_level())ob_start(null,4096);$T=strip_tags($T);$Ok=$db!==false&&$db!==null&&SERVER!=""?" - ".h(Admin::get()->getServerName(SERVER)):"";$Qk=strip_tags(Admin::get()->getServiceTitle());$mm=$T.$Ok." - ".($Qk!=""?$Qk:"AdminNeo");echo"<!DOCTYPE html>\n","<html lang='",Locale::get()->getLanguage(),"' dir='",lang(132),"' class='",lang(132)," nojs'>\n","<head>\n","<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n","<meta name='robots' content='noindex, nofollow'>\n","<meta name='viewport' content='width=device-width, initial-scale=1'>\n","<title>$mm</title>\n";$Fb=validate_color_variant(Admin::get()->getConfig()->getColorVariant());echo"<link rel='stylesheet' href='",link_files("default-$Fb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("default-$Fb-dark.css",[]),"'>\n";$fm=Admin::get()->getSettings()->getTheme();list($fm,$Fb)=validate_theme($fm,$Fb);if($fm!="default"){echo"<link rel='stylesheet' href='",link_files("$fm-$Fb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("$fm-$Fb-dark.css",[]),"'>\n";}foreach(Admin::get()->getCssUrls()as$Sm){if(strpos($Sm,"adminneo-dark.css")===0&&!Admin::get()->isDarkModeForced())echo"<link rel='stylesheet' media='(prefers-color-scheme: dark)' href='",h($Sm),"'>\n";else
echo"<link rel='stylesheet' href='",h($Sm),"'>\n";}$Gh=Admin::get()->getSettings()->getNavigationWidth();echo"<style id='navigation-width'>";if($Gh)echo"@media screen and (min-width: 1024px) { :root { --menu-width: ",sprintf("%.2F",$Gh),"rem } }";echo"</style>\n",script_src(link_files("main.js",[]));foreach(Admin::get()->getJsUrls()as$Sm)echo
script_src($Sm);Admin::get()->printFavicons();Admin::get()->printToHead();echo"</head>\n","<body>\n",script("const offlineMessage = '".js_escape(lang(133))."';\n"."const thousandsSeparator = '".js_escape(lang(108))."';"),"<div id='help' class='jush-".DIALECT." jsonly hidden'></div>",script("initHelpPopup();"),'<menu class="access-menu">','<li><a href="#main-content">'.lang(134).'</a></li>','<li><a class="panel-link" href="#navigation-panel">'.lang(135).'</a></li>';if($db!==null&&DB!=""&&$_GET["ns"]!=="")echo'<li><a class="panel-link" href="#tables">'.lang(136).'</a></li>';echo'</menu>',"<div id='content'>\n","<div class='header'>\n","<button id='open-navigation-button' type='button' class='button light navigation-button' title='",lang(137),"' aria-controls='navigation-panel' aria-expanded='false'>",icon_solo("menu"),"</button>";if($db!==null){echo'<nav class="breadcrumbs"><ul>',"<li><a href='".h(HOME_URL)."' title='",lang(138),"'>",icon_solo("home"),"</a></li>";$Mk=h(Admin::get()->getServerName((string)SERVER));if($db===false)echo"<li>$Mk</li>";else{$x=substr(preg_replace('~\b(db|ns)=[^&]*&~','',ME),0,-1);echo"<li><a href='".h($x)."' accesskey='1' title='Alt+Shift+1'>$Mk</a></li>";if($_GET["ns"]!=""||(DB!=""&&is_array($db)))echo'<li><a href="'.h($x."&db=".urlencode(DB).(support("scheme")?"&ns=":"")).'">'.h(DB).'</a></li>';if($db===true){if($_GET["ns"]!="")echo'<li>'.h($_GET["ns"]).'</li>';else
echo"<li>",h(DB),"</li>";}else{if($_GET["ns"]!="")echo'<li><a href="'.h(substr(ME,0,-1)).'">'.h($_GET["ns"]).'</a></li>';foreach($db
as$u=>$X){if(is_string($u)){$Hc=(is_array($X)?$X[1]:h($X));if($Hc!="")echo"<li><a href='".h(ME."$u=").urlencode(is_array($X)?$X[0]:$X)."'>$Hc</a></li>";}else
echo"<li>$X</li>\n";}}}echo"</ul></nav>";}echo"</div>\n","<div id='main-content'>\n","<h1>$T</h1>\n","<div id='ajaxstatus' role='status' class='jsonly'></div>\n";restart_session();page_messages();$g=&get_session("dbs");if(DB!=""&&$g&&!in_array(DB,$g,true))$g=null;stop_session();define("AdminNeo\PAGE_HEADER",1);ob_flush();flush();}function
validate_color_variant($Fb){list(,$Fb)=validate_theme("default",$Fb);return$Fb;}function
validate_theme($fm,$Fb){$gm=get_available_themes();if(isset($gm[$fm][$Fb]))return[$fm,$Fb];if(isset($gm["default"][$Fb]))return["default",$Fb];reset($gm["default"]);return["default",key($gm["default"])];}function
get_available_themes(){return
array('default'=>array('blue'=>true,),);}function
get_theme_titles($Fb){$gm=get_available_themes();$nm=[];foreach($gm
as$fm=>$Gb){if(isset($Gb[$Fb])?$Gb[$Fb]:false)$nm[$fm]=($fm=="default"?"Neo":ucwords(str_replace("-"," ",$fm)));}return$nm;}function
page_headers(){header("Content-Type: text/html; charset=utf-8");header("Cache-Control: no-cache");header("X-XSS-Protection: 0");header("X-Content-Type-Options: nosniff");header("Referrer-Policy: origin-when-cross-origin");header("X-Frame-Options: DENY");$kc=["script-src"=>"'self' 'unsafe-inline' 'nonce-".get_nonce()."' 'strict-dynamic'","connect-src"=>"'self' https://api.github.com/repos/adminneo-org/adminneo/releases/latest","frame-src"=>"'self'","object-src"=>"'none'","base-uri"=>"'none'","form-action"=>"'self'",];Admin::get()->updateCspHeader($kc);$Nc=[];foreach($kc
as$Mc=>$fl)$Nc[]="$Mc $fl";header("Content-Security-Policy: ".implode("; ",$Nc));Admin::get()->sendHeaders();}function
get_nonce(){static$Qh;if(!$Qh)$Qh=Random::strongKey();return$Qh;}function
page_messages(){$Rm=preg_replace('~^[^?]*~','',$_SERVER["REQUEST_URI"]);$lh=isset($_SESSION["messages"][$Rm])?$_SESSION["messages"][$Rm]:null;if($lh){foreach($lh
as$hh)echo"<div class='message'>$hh</div>\n",script("initToggles(qsl('.message'));");unset($_SESSION["messages"][$Rm]);}foreach(Admin::get()->getErrors()as$j)echo"<div class='error'>$j</div>\n";}function
page_footer($rh=null){echo"</div>\n","</div>\n","<div id='navigation-panel' class='navigation-panel'>\n","<div class='focus-trap-begin'></div>\n";$ug=isset($_COOKIE["neo_version"])?$_COOKIE["neo_version"]:null;echo"<div class='header'>\n","<button id='close-navigation-button' type='button' class='button light navigation-button' title='",lang(139),"' aria-controls='navigation-panel'>",icon_solo("close"),"</button>",Admin::get()->getServiceTitle()."\n";if($rh!="auth"){echo"<span class='version'>",h(preg_replace('~\\.0(-|$)~','$1',VERSION));if(Admin::get()->getConfig()->isVersionVerificationEnabled()&&$ug&&version_compare(VERSION,$ug)<0)echo"<a id='version' class='version-badge' href='https://www.adminneo.org/download' ".target_blank()." title='".h($ug)."'>",icon_solo("asterisk"),"</a>";echo"</span>\n";if(Admin::get()->getConfig()->isVersionVerificationEnabled()&&!$ug)echo
script("verifyVersion();");}echo"</div>\n";Admin::get()->printNavigation($rh);echo"<div class='footer'>\n","<div class='toolbox'>";if($rh=="auth")language_select();else{$x=h(preg_replace('~\b(db|ns)=[^&]*&~',"",ME)."settings=");echo"<a class='button light' title='",lang(140),"' href='$x'>",icon_solo("settings"),"</a>";}echo"</div>";if($rh!="auth")Admin::get()->printLogout();echo"</div>\n","<div id='navigation-resizer' class='navigation-resizer'></div>\n","<div class='focus-trap-end'></div>\n","</div>\n",script("initNavigation(); initNavigationResizer('".js_escape(ME)."set=navigation-width', '".get_token()."', ".Settings::$NavigationWidthMin.", ".Settings::$NavigationWidthMax.");");}function
int32($Bh){while($Bh>=2147483648)$Bh-=4294967296;while($Bh<=-2147483649)$Bh+=4294967296;return(int)$Bh;}function
long2str(array$W,$rn){$mk='';foreach($W
as$X)$mk
.=pack('V',$X);return$rn?substr($mk,0,end($W)):$mk;}function
str2long($mk,$rn){$W=array_values(unpack('V*',str_pad($mk,4*ceil(strlen($mk)/4),"\0")));if($rn)$W[]=strlen($mk);return$W;}function
xxtea_mx($Dn,$Cn,$zl,$cg){return
int32((($Dn>>5&0x7FFFFFF)^$Cn<<2)+(($Cn>>3&0x1FFFFFFF)^$Dn<<4))^int32(($zl^$Cn)+($cg^$Dn));}function
xxtea_encrypt_string($ij,$u){$u=array_values(unpack("V*",pack("H*",md5($u))));$W=str2long($ij,true);$Bh=count($W)-1;$Dn=$W[$Bh];$Cn=$W[0];$Ej=floor(6+52/($Bh+1));$zl=0;while($Ej-->0){$zl=int32($zl+0x9E3779B9);$jd=$zl>>2&3;for($Ki=0;$Ki<$Bh;$Ki++){$Cn=$W[$Ki+1];$_h=xxtea_mx($Dn,$Cn,$zl,$u[$Ki&3^$jd]);$Dn=int32($W[$Ki]+$_h);$W[$Ki]=$Dn;}$Cn=$W[0];$_h=xxtea_mx($Dn,$Cn,$zl,$u[$Ki&3^$jd]);$Dn=int32($W[$Bh]+$_h);$W[$Bh]=$Dn;}return
long2str($W,false);}function
xxtea_decrypt_string($f,$u){$u=array_values(unpack("V*",pack("H*",md5($u))));$W=str2long($f,false);$Bh=count($W)-1;$Dn=$W[$Bh];$Cn=$W[0];$Ej=floor(6+52/($Bh+1));$zl=int32($Ej*0x9E3779B9);while($zl){$jd=$zl>>2&3;for($Ki=$Bh;$Ki>0;$Ki--){$Dn=$W[$Ki-1];$_h=xxtea_mx($Dn,$Cn,$zl,$u[$Ki&3^$jd]);$Cn=int32($W[$Ki]-$_h);$W[$Ki]=$Cn;}$Dn=$W[$Bh];$_h=xxtea_mx($Dn,$Cn,$zl,$u[$Ki&3^$jd]);$Cn=int32($W[0]-$_h);$W[0]=$Cn;$zl=int32($zl-0x9E3779B9);}return
long2str($W,true);}const
ENCRYPTION_GCM='aes-256-gcm';const
ENCRYPTION_CBC='aes-256-cbc';const
ENCRYPTION_TAG_LENGTH=16;const
ENCRYPTION_HMAC_LENGTH=64;function
generate_iv($v){if(function_exists('random_bytes')){try{return
random_bytes($v);}catch(Exception$jd){}}return
openssl_random_pseudo_bytes($v);}function
hash_key($u){return
substr(hash('sha512',$u,true),0,32);}function
aes_encrypt_string($ij,$u){$ph=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$u=hash_key($u);$Xf=generate_iv(openssl_cipher_iv_length($ph)?:16);if($ph==ENCRYPTION_GCM)$yb=openssl_encrypt($ij,$ph,$u,OPENSSL_RAW_DATA,$Xf,$Wl,"",ENCRYPTION_TAG_LENGTH);else{$yb=openssl_encrypt($ij,$ph,$u,OPENSSL_RAW_DATA,$Xf);$Wl=hash_hmac("sha512",$Xf.$yb,$u,true);}if($yb===false)return
false;return$Xf.$Wl.$yb;}function
aes_decrypt_string($f,$u){$ph=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$Yf=openssl_cipher_iv_length($ph)?:16;$Xl=$ph==ENCRYPTION_GCM?ENCRYPTION_TAG_LENGTH:ENCRYPTION_HMAC_LENGTH;if(strlen($f)<$Yf+$Xl)return
false;$u=hash_key($u);$Xf=substr($f,0,$Yf);$Wl=substr($f,$Yf,$Xl);$yb=substr($f,$Yf+$Xl);if($Xf===false||$Wl===false||$yb===false)return
false;if($ph==ENCRYPTION_GCM)return
openssl_decrypt($yb,$ph,$u,OPENSSL_RAW_DATA,$Xf,$Wl);else{$ef=hash_hmac('sha512',$Xf.$yb,$u,true);if(!hash_equals($Wl,$ef))return
false;return
openssl_decrypt($yb,$ph,$u,OPENSSL_RAW_DATA,$Xf);}}function
encrypt_string($ij,$u){if($ij=="")return"";if(extension_loaded('openssl'))return
aes_encrypt_string($ij,$u);else
return
xxtea_encrypt_string($ij,$u);}function
decrypt_string($f,$u){if($f=="")return"";if(extension_loaded('openssl'))return
aes_decrypt_string($f,$u);else
return
xxtea_decrypt_string($f,$u);}$ej=[];if($_COOKIE["neo_permanent"]){foreach(explode(" ",$_COOKIE["neo_permanent"])as$X){list($u)=explode(":",$X);$ej[$u]=$X;}}function
validate_server_input(array&$ej){$N=preg_replace('~:/[-\w.][-\w.:/]*$~D',"",SERVER);if($N=="")return;if(!preg_match('~^[^:]+://~',$N))$N="https://$N";$Yi=parse_url($N);if(!$Yi)auth_error($ej);if(isset($Yi['user'])||isset($Yi['pass'])||isset($Yi['query'])||isset($Yi['fragment']))auth_error($ej);if(isset($Yi['scheme'])&&!preg_match('~^(https?)$~i',$Yi['scheme']))auth_error($ej);$hf=$Yi['host'].(isset($Yi['path'])?$Yi['path']:'');if(!is_server_host_valid($hf))auth_error($ej);if(isset($Yi['port'])&&($Yi['port']<1024||$Yi['port']>65535))auth_error($ej,lang(141));}if(!function_exists('AdminNeo\is_server_host_valid')){function
is_server_host_valid($hf){return
strpos($hf,'/')===false;}}function
build_http_url($N,$V,$F,$Ac,$_c=null){if(!preg_match('~^(https?://)?([^:]*)(:\d+)?$~',rtrim($N,'/'),$_))return
null;return($_[1]?:"http://").($V!==""||$F!==""?urlencode($V).":".urlencode($F)."@":"").($_[2]!==""?$_[2]:$Ac).(isset($_[3])?$_[3]:($_c?":$_c":""));}function
add_invalid_login(){$Xa=get_temp_dir()."/adminneo-invalid";$m=null;foreach(glob("$Xa*")?:[$Xa]as$n){$m=open_file_with_lock($n);if($m)break;}if(!$m){$m=open_file_with_lock("$Xa-".Random::strongKey());if(!$m)return;}$Lf=json_decode(stream_get_contents($m),true);$im=time();if($Lf){foreach($Lf
as$Mf=>$X){if($X[0]<$im)unset($Lf[$Mf]);}}$Kf=&$Lf[Admin::get()->getBruteForceKey()];if(!$Kf)$Kf=[$im+30*60,0];$Kf[1]++;write_and_unlock_file($m,json_encode($Lf));}function
check_invalid_login(array&$ej){$Xa=get_temp_dir()."/adminneo-invalid";$Lf=[];foreach(glob("$Xa*")as$n){$m=open_file_with_lock($n);if($m){$Lf=json_decode(stream_get_contents($m),true);unlock_file($m);break;}}$Kf=($Lf?$Lf[Admin::get()->getBruteForceKey()]:[]);$Oh=($Kf&&$Kf[1]>29?$Kf[0]-time():0);if($Oh>0)auth_error($ej,lang(142,ceil($Oh/60)));}function
connect_to_db(array&$ej){if(Admin::get()->getConfig()->hasServers()&&!Admin::get()->getConfig()->getServer(SERVER))auth_error($ej);$d=connect(true,$j);if(!$d)connection_error(nl2br(h($j)),$ej);return$d;}function
authenticate(array&$ej){$I=Admin::get()->authenticate($_GET["username"],get_password());if($I!==true)connection_error($I,$ej);}function
connection_error($j,array&$ej){$j=$j?:lang(3);if(preg_match('~^ +| +$~',get_password()))$j
.="<br>".lang(143);auth_error($ej,$j);}Admin::get()->init();$Na=isset($_POST["auth"])?$_POST["auth"]:null;if($Na){session_regenerate_id();$N=isset($Na["server"])?$Na["server"]:"";$Nk=Admin::get()->getConfig()->getServer($N);$Zc=$Nk?$Nk->getDriver():(isset($Na["driver"])?$Na["driver"]:"");$N=$Nk?$N:trim($N);$V=isset($Na["username"])?$Na["username"]:"";$F=isset($Na["password"])?$Na["password"]:"";if($Nk&&$Nk->hasCredentials()&&$V==""&&$F==""){$V=$Nk->getUsername();$F=$Nk->getPassword();}$h=$Nk?$Nk->getDatabase():(isset($Na["db"])?$Na["db"]:"");save_login($Zc,$N,$V,$F,$h);if($Na["permanent"]){$u=implode("-",array_map("base64_encode",[$Zc,$N,$V,$h]));$yj=Admin::get()->getPrivateKey(true);$ud=$yj?encrypt_string($F,$yj):false;$ej[$u]="$u:".base64_encode($ud?:"");cookie("neo_permanent",implode(" ",$ej));}if(count($_POST)==1||DRIVER!=$Zc||SERVER!=$N||$_GET["username"]!==$V||DB!=$h)redirect(auth_url($Zc,$N,$V,$h));}elseif($_POST["logout"]&&(!$_SESSION["token"]||verify_token())){foreach(["pwds","db","dbs","queries"]as$u)set_session($u,null);unset_permanent($ej);redirect(SERVER_HOME_URL,lang(144));}elseif($ej&&!$_SESSION["pwds"]){session_regenerate_id();$yj=Admin::get()->getPrivateKey();foreach($ej
as$u=>$X){list(,$xb)=explode(":",$X);list($Zc,$N,$V,$h)=array_map("base64_decode",explode("-",$u));$F=$yj?decrypt_string(base64_decode($xb),$yj):false;save_login($Zc,$N,$V,$F,$h);}}function
unset_permanent(array&$ej){foreach($ej
as$u=>$X){list($Zc,$N,$V,$h)=array_map("base64_decode",explode("-",$u));if($Zc==DRIVER&&$N==SERVER&&$V==$_GET["username"]&&$h==DB)unset($ej[$u]);}cookie("neo_permanent",implode(" ",$ej));}function
auth_error(array&$ej,$j=null){$Rk=session_name();if(isset($_GET["username"])){header("HTTP/1.1 403 Forbidden");if(($_COOKIE[$Rk]||$_GET[$Rk])&&!$_SESSION["token"])$j=lang(145);else{restart_session();add_invalid_login();$F=get_password();if($F!==null){if($F===false)$j=lang(146);delete_login(DRIVER,SERVER,$_GET["username"]);}unset_permanent($ej);}}if(!$_COOKIE[$Rk]&&$_GET[$Rk]&&ini_bool("session.use_only_cookies"))$j=lang(147);if(!$j)$j=lang(3);Admin::get()->addError($j);print_login_page();}function
print_login_page(){$Ni=session_get_cookie_params();cookie("neo_key",($_COOKIE["neo_key"]?:Random::strongKey()),$Ni["lifetime"]);if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);page_header(lang(32),null);echo"<form action='' method='post'>\n","<div>";if(print_hidden_fields($_POST,["auth"]))echo"<p class='message'>".lang(148)."\n";echo"</div>\n";Admin::get()->printLoginForm();echo"</form>\n";page_footer("auth");exit;}if(isset($_GET["username"])&&!DRIVER)print_login_page();if(isset($_GET["username"])&&!defined('AdminNeo\DRIVER_EXTENSION')){Admin::get()->addError(lang(149,implode(", ",Drivers::getExtensions(DRIVER))));unset($_SESSION["pwds"][DRIVER]);unset_permanent($ej);page_header(lang(150),false);page_footer("auth");exit;}if(!isset($_GET["username"])||get_password()===null)print_login_page();validate_server_input($ej);check_invalid_login($ej);Admin::get()->getConfig()->applyServer(SERVER);$d=connect_to_db($ej);authenticate($ej);create_driver($d);if($_POST["logout"]&&$_SESSION["token"]&&!verify_token()){Admin::get()->addError(lang(151));page_header(lang(7));page_footer("db");exit;}if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);stop_session(true);if($Na&&$_POST["token"])$_POST["token"]=get_token();if($_POST){if(!verify_token()){Admin::get()->addError(lang(151).' '.lang(152));$_POST=[];}}elseif($_SERVER["REQUEST_METHOD"]=="POST"){$j=lang(153,"'post_max_size'");if(isset($_GET["sql"]))$j
.=' '.lang(154);Admin::get()->addError($j);}if(isset($_GET["settings"])){$O=Admin::get()->getSettings();$Uk=array_merge(Admin::get()->getSettingsRows(1),Admin::get()->getSettingsRows(2),Admin::get()->getSettingsRows(3));if($_POST){$Ni=[];foreach($Uk
as$u=>$K){if(isset($_POST[$u])){$Um=$_POST[$u]===""||(is_array($_POST[$u])&&in_array("",$_POST[$u]));$Ni[$u]=(!$Um?$_POST[$u]:null);}}$O->updateParameters($Ni);redirect(remove_from_uri());}$T=lang(140);page_header($T,[$T]);echo"<form id='settings' action='' method='post'>\n","<table class='box'>\n";foreach($Uk
as$K)echo$K;echo"</table>\n","<p>","<input type='submit' value='".lang(116),"' class='button default hidden'>",input_token(),"</p>\n","</form>\n",script("initSettingsForm();");page_footer();exit;}if(isset($_GET["status"]))$_GET["variables"]=$_GET["status"];if(isset($_GET["import"]))$_GET["sql"]=$_GET["import"];if(DB==""&&isset($_GET["ns"]))redirect(remove_from_uri('ns'));if(!(DB!=""?Connection::get()->selectDatabase(DB):(isset($_GET["sql"])||isset($_GET["dump"])||isset($_GET["database"])||isset($_GET["processlist"])||isset($_GET["privileges"])||isset($_GET["user"])||isset($_GET["variables"])||$_GET["script"]=="connect"||$_GET["script"]=="kill"))){if(DB!=""||$_GET["refresh"]){restart_session();set_session("dbs",null);}if(DB!=""){Admin::get()->addError(lang(155));header("HTTP/1.1 404 Not Found");page_header(lang(31).": ".h(DB),true);}else{if($_POST["db"])queries_redirect(substr(ME,0,-1),lang(156),drop_databases($_POST["db"]));$T=h(Drivers::get(DRIVER).": ".Admin::get()->getServerName(SERVER));page_header($T,false);$Hg=['privileges'=>[lang(73),"users"],'processlist'=>[lang(157),"list"],'variables'=>[lang(158),"variable"],'status'=>[lang(159),"status"],];$Ig="";foreach($Hg
as$u=>$X){if(support($u))$Ig
.="<a href='".h(ME)."$u='>".icon($X[1])."$X[0]</a>";}if($Ig)echo"<p class='links top-links'>$Ig</p>\n";echo"<p>".lang(160,Drivers::get(DRIVER),"<b>".h(Connection::get()->getVersion())."</b>","<b>".DRIVER_EXTENSION."</b>")."\n","<p>".lang(161,"<b>".h(logged_user())."</b>")."\n";$g=Admin::get()->getDatabases();if($g){$wk=support("scheme");$Da=collations();echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr>".(support("database")?"<th>":"")."<th aria-sort='ascending'>".lang(31).(get_session("dbs")!==null?" - <a href='".h(ME)."refresh=1'>".lang(162)."</a>":"")."<td>".lang(47)."<td>".lang(163)."<td>".lang(164)." - <a href='".h(ME)."dbsize=1'>".lang(165)."</a>".script("qsl('a').onclick = partial(ajaxSetHtml, '".js_escape(ME)."script=connect');","")."</thead>\n","<tbody>\n";$g=($_GET["dbsize"]?count_tables($g):array_flip($g));foreach($g
as$h=>$S){$hk=h(ME)."db=".urlencode($h);$r=h("Db-".$h);echo"<tr>".(support("database")?"<th class='actions'>".checkbox("db[]",$h,in_array($h,(array)$_POST["db"]),"","","",$r):""),"<th><a href='$hk' id='$r'>".h($h)."</a>";$Cb=h(db_collation($h,$Da));echo"<td>".(support("database")?"<a href='$hk".($wk?"&amp;ns=":"")."&amp;database=' title='".lang(70)."'>$Cb</a>":$Cb),"<td align='right'><a href='$hk&amp;schema=' id='tables-".h($h)."' title='".lang(72)."'>".($_GET["dbsize"]?$S:"?")."</a>","<td align='right' id='size-".h($h)."'>".($_GET["dbsize"]?db_size($h):"?"),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});"),"</table>\n","</div>\n";if(support("database"))echo"<div class='table-footer'><div class='field-sets'>\n","<fieldset><legend>",lang(166)," <span id='selected'></span></legend><div class='fieldset-content'>\n",input_hidden("all"),script("qsl('input').onclick = countDbs;"),"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(),"\n","</div></fieldset>\n","</div></div>\n",script("initTableFooter()");echo"</div>\n",input_token(),"</form>\n",script("tableCheck();");}}echo'<p class="links"><a href="'.h(ME).'database=">'.icon("database-add").lang(76)."</a>\n";page_footer("db");exit;}if(isset($_GET["select"])&&($_POST["edit"]||$_POST["clone"])&&!$_POST["save"])$_GET["edit"]=$_GET["select"];if(isset($_GET["callf"]))$_GET["call"]=$_GET["callf"];if(isset($_GET["function"]))$_GET["procedure"]=$_GET["function"];if(isset($_GET["download"])){$a=$_GET["download"];$l=fields($a);header("Content-Type: application/octet-stream");header("Content-Disposition: attachment; filename=".friendly_url("$a-".implode("_",$_GET["where"])).".".friendly_url($_GET["field"]));$M=[idf_escape($_GET["field"])];$I=Driver::get()->select($a,$M,[where($_GET,$l)],$M);$K=($I?$I->fetchRow():[]);echo
Connection::get()->formatValue($K[0],$l[$_GET["field"]]);exit;}elseif(isset($_GET["table"])){$a=$_GET["table"];$l=fields($a);if(!$l)Admin::get()->addError(error()?:lang(79));$R=table_status1($a,true);$A=Admin::get()->getTableName($R);$gk=[];foreach($l
as$u=>$k)$gk+=$k["privileges"];$T=$l&&is_view($R)?$R['Engine']=='materialized view'?lang(168):lang(169):lang(9);$Ml=$A!=""?$A:h($a);page_header("$T: $Ml",[$Ml]);$Ef=null;if(isset($gk["insert"])||!support("table"))$Ef=[];Admin::get()->printTableMenu($R,$Ef);$xf=[];if(!preg_match("~sqlite|mssql|pgsql~",DIALECT)&&isset($R["Engine"]))$xf[]=lang(170).": ".h($R["Engine"]);if(isset($R["Collation"]))$xf[]=lang(47).": ".h($R["Collation"]);if($xf)echo"<p>",implode(", ",$xf),"</p>";if($l)Admin::get()->printTableStructure($l);$Mb=$R["Comment"];if($Mb!="")echo"<p class='keep-lines'>",lang(48),": ",Admin::get()->formatComment($Mb),"</p>\n";if(!is_view($R))$ld='<a href="'.h(ME).'create='.urlencode($a).'">'.icon("edit").lang(36)."</a>";elseif(support("view"))$ld='<a href="'.h(ME).'view='.urlencode($a).'">'.icon("edit").lang(37)."</a>";else$ld="";if($ld&&($xf||$l||$Mb!="")){echo"<p id='table-links' class='links'>",$ld;$tl=Admin::get()->getStructureDumpFormats();if($tl){$ec=Admin::get()->getSettings()->getParameter("copyFormat");if(!isset($tl[$ec]))$ec=key($tl);echo"<span>","<a id='copy-table-structure' href='".h(ME)."dump=".urlencode($a)."&format=".urlencode($ec)."' class='jsonly'>".icon("copy").lang(171,h($tl[$ec]))."</a>";$Ak=count($tl)>1;if($Ak)echo"<a href='#format-list' class='toggle jsonly' title='".lang(172)."'>".icon_chevron_down()."</a>";echo"</span></p>\n";if($Ak){echo"<ul id='format-list' class='select-menu hidden'>";foreach($tl
as$se=>$A)echo"<li><a href='".h(ME)."dump=".urlencode($a)."&format=".urlencode($se)."'>".h($A)."</a></li>";echo"</ul>",script("initToggles(gid('table-links'));");}echo
script("initCopyTableStructure();");}else
echo"</p>\n";}$Oi=Driver::get()->getParentTables($a);if($Oi){echo"<h2>".lang(173)."</h2>\n";Admin::get()->printRelatedTables($Oi);}if(Driver::get()->getPartitionBy()&&str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned")){$Xi=Driver::get()->getPartitionsInfo($a);if($Xi){echo"<h2 id='partitions'>".lang(50)."</h2>\n";Admin::get()->printTablePartitions($Xi);if(DIALECT!="pgsql"&&$ld)echo"<p class='links'>$ld</p>\n";}}$yf=Driver::get()->getInheritedTables($a);if($yf){echo"<h2 id='inherited-by'>".lang(174)."</h2>\n";Admin::get()->printRelatedTables($yf);}if(support("indexes")&&Driver::get()->supportsIndex($R)){echo"<h2 id='indexes'>".lang(175)."</h2>\n";$t=indexes($a);if($t)Admin::get()->printTableIndexes($t,$R);if(Driver::get()->supportsAlterIndex($R))echo'<p class="links"><a href="'.h(ME).'indexes='.urlencode($a).'">'.icon("edit").lang(176)."</a>\n";}if(!is_view($R)){if(fk_support($R)){echo"<h2 id='foreign-keys'>".lang(91)."</h2>\n";$qe=foreign_keys($a);if($qe){echo"<table>\n","<thead><tr><th>".lang(177)."<td>".lang(178)."<td>".lang(94)."<td>".lang(93)."<td></thead>\n";foreach($qe
as$A=>$o)echo"<tr title='".h($A)."'>","<th><i>".implode("</i>, <i>",array_map('AdminNeo\h',$o["source"]))."</i>","<td><a href='".h($o["db"]!=""?preg_replace('~db=[^&]*~',"db=".urlencode($o["db"]),ME):($o["ns"]!=""?preg_replace('~ns=[^&]*~',"ns=".urlencode($o["ns"]),ME):ME))."table=".urlencode($o["table"])."'>".($o["db"]!=""&&$o["db"]!=DB?"<b>".h($o["db"])."</b>.":"").($o["ns"]!=""&&$o["ns"]!=$_GET["ns"]?"<b>".h($o["ns"])."</b>.":"").h($o["table"])."</a>","(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$o["target"]))."</i>)","<td>".h($o["on_delete"]),"<td>".h($o["on_update"]),'<td><a href="'.h(ME.'foreign='.urlencode($a).'&name='.urlencode($A)).'">'.lang(179).'</a>',"\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'foreign='.urlencode($a).'">'.icon("add").lang(180)."</a>\n";}if(support("check")){echo"<h2 id='checks'>".lang(181)."</h2>\n";$sb=Driver::get()->checkConstraints($a);if($sb){echo"<table cellspacing='0'>\n";foreach($sb
as$u=>$X)echo"<tr title='".h($u)."'>","<td><code class='jush-".DIALECT."'>".truncate_utf8(preg_replace('~\s+~',' ',ltrim($X))),"<td><a href='".h(ME.'check='.urlencode($a).'&name='.urlencode($u))."'>".lang(179)."</a>","\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'check='.urlencode($a).'">'.icon("add").lang(182)."</a>\n";}}if(support(is_view($R)?"view_trigger":"trigger")){echo"<h2 id='triggers'>".lang(183)."</h2>\n";$zm=triggers($a);if($zm){echo"<table>\n";foreach($zm
as$u=>$X)echo"<tr><td>".h($X[0])."<td>".h($X[1])."<th>".h($u)."<td><a href='".h(ME.'trigger='.urlencode($a).'&name='.urlencode($u))."'>".lang(179)."</a>\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'trigger='.urlencode($a).'">'.icon("add").lang(184)."</a>\n";}}elseif(isset($_GET["schema"])){$lm=h(": ".DB.($_GET["ns"]?".$_GET[ns]":""));page_header(lang(72).$lm,[lang(72)]);$Ol=[];$Pl=[];$Zd=[];$pa=($_GET["schema"]?:$_COOKIE["neo_schema-".str_replace(".","_",DB)]);preg_match_all('~([^:]+):([-0-9.]+)x([-0-9.]+)(_|$)~',$pa,$_,PREG_SET_ORDER);foreach($_
as$z){$Ol[$z[1]]=[(float)$z[2],(float)$z[3]];$Pl[]="\n'".js_escape($z[1])."': [ $z[2], $z[3] ]";}$Eg=1.4;$rm=0;$Wa=-1;$uk=[];$Tj=[];$xg=[];$Ea=Driver::get()->getAllFields();foreach(table_status('',true)as$Q=>$R){if(is_view($R))continue;$G=0;$uk[$Q]["fields"]=[];foreach(isset($Ea[$Q])?$Ea[$Q]:[]as$k){$G=round($G+$Eg,2);$Zd[$Q][$k["field"]]=$G;$uk[$Q]["fields"][$k["field"]]=$k;}$uk[$Q]["pos"]=(isset($Ol[$Q])?$Ol[$Q]:[$rm,0]);foreach(Admin::get()->getForeignKeys($Q)as$X){if(!$X["db"]){$vg=$Wa;if((isset($Ol[$Q][1])?$Ol[$Q][1]:0)||(isset($Ol[$X["table"]][1])?$Ol[$X["table"]][1]:0))$vg=min(floatval(isset($Ol[$Q][1])?$Ol[$Q][1]:0),floatval(isset($Ol[$X["table"]][1])?$Ol[$X["table"]][1]:0))-1;else$Wa-=.1;while($xg[(string)$vg])$vg-=.0001;$uk[$Q]["references"][$X["table"]][(string)$vg]=[$X["source"],$X["target"]];$Tj[$X["table"]][$Q][(string)$vg]=$X["target"];$xg[(string)$vg]=true;}}$rm=max($rm,round($uk[$Q]["pos"][0]+$Eg+$G+1,2));}echo"<div id='schema' style='height: {$rm}em;'>\n";foreach($uk
as$A=>$Q){$Ye=round($Eg+count($Q["fields"])*$Eg+0.4,2);echo"<div class='table' style='top: ".$Q["pos"][0]."em; left: ".$Q["pos"][1]."em; height: {$Ye}em;'>","<div class='content'>",'<h4><a href="'.h(ME).'table='.urlencode($A).'">'.h($A)."</a></h4>","<ul>";foreach($Q["fields"]as$k){$X='<span '.type_class($k["type"]).' title="'.h($k["type"].($k["length"]?"($k[length])":"").($k["null"]?" NULL":'')).'">'.h($k["field"]).'</span>';echo"<li>".($k["primary"]?"<i>$X</i>":$X)."</li>";}echo"</ul>","</div>";foreach((array)$Q["references"]as$Zl=>$Vj){foreach($Vj
as$vg=>$Pj){$wg=$vg-(isset($Ol[$A][1])?$Ol[$A][1]:0);$q=0;foreach($Pj[0]as$el){echo"\n<div class='references outgoing' title='",h($Zl),"' id='refs$vg-$q' style='left: {$wg}em; top: ",$Zd[$A][$el],"em;'>","<div style='width: ".(-$wg)."em;'></div>","</div>";$q++;}}}foreach((array)$Tj[$A]as$Zl=>$Vj){foreach($Vj
as$vg=>$c){$wg=$vg-(isset($Ol[$A][1])?$Ol[$A][1]:0);$q=0;foreach($c
as$Yl){echo"\n<div class='references incoming' title='",h($Zl),"' id='refd$vg-$q' style='left: {$wg}em; top: ".$Zd[$A][$Yl]."em;'>","<svg viewBox='0 0 22 22' fill='currentColor'><path d='M11,19l10,-8l-10,-8l0,16Z'/></svg>","<div style='width: ".(-$wg)."em;'></div>","</div>";$q++;}}}echo"\n</div>\n";}foreach($uk
as$A=>$Q){foreach((array)$Q["references"]as$Zl=>$Vj){if($uk[$Zl]){foreach($Vj
as$vg=>$Pj){$qh=$rm;$Zg=-10;foreach($Pj[0]as$u=>$el){$oj=$Q["pos"][0]+$Zd[$A][$el];$pj=$uk[$Zl]["pos"][0]+$Zd[$Zl][$Pj[1][$u]];$qh=round(min($qh,$oj,$pj),2);$Zg=round(max($Zg,$oj,$pj),2);}echo"<div class='references vertical' id='refl$vg' style='left: $vg"."em; top: $qh"."em;'>"."<div style='height: ".round($Zg-$qh,2)."em;'></div></div>\n";}}}}echo"</div>\n",script("initSchema('".js_escape(DB)."', $rm, {".implode(",",$Pl)."})"),"<p class='links'>","<a href='",(ME."schema=".urlencode($pa)),"' id='schema-link'>",lang(185),"</a>","</p>\n";}elseif(isset($_GET["dump"])){$a=$_GET["dump"];$O=Admin::get()->getSettings();$tl=Admin::get()->getStructureDumpFormats();$qc=Admin::get()->getDataDumpFormats();$fc=is_ajax()&&$a!="";if($fc){$ec=isset($_GET["format"])?$_GET["format"]:null;if(!isset($tl[$ec])){reset($tl);$ec=key($tl);}$O->updateParameter("copyFormat",$ec);$_POST=["format"=>$ec,"output"=>"text","db_style"=>"","table_style"=>"CREATE","tables"=>[$a],"triggers"=>(support("trigger")?"1":""),"data_style"=>"",];}if($_POST){$id=isset($tl[$_POST["format"]]);$hd=isset($qc[$_POST["format"]]);if(!$fc)$O->updateParameters(["dumpFormat"=>$_POST["format"],"dumpOutput"=>$_POST["output"],]+($id?["dumpDbStyle"=>$_POST["db_style"],"dumpTypes"=>isset($_POST["types"])?$_POST["types"]:(support("type")?"":null),"dumpRoutines"=>isset($_POST["routines"])?$_POST["routines"]:(support("routine")?"":null),"dumpEvents"=>isset($_POST["events"])?$_POST["events"]:(support("event")?"":null),"dumpTableStyle"=>$_POST["table_style"],"dumpAutoIncrement"=>isset($_POST["auto_increment"])?$_POST["auto_increment"]:"","dumpTriggers"=>isset($_POST["triggers"])?$_POST["triggers"]:(support("trigger")?"":null),]:[])+($hd?["dumpDataStyle"=>$_POST["data_style"],]:[]));$Rl=($id?$_POST["table_style"]:"CREATE");$sc=($hd?$_POST["data_style"]:"");if(DB!="")$g=[DB];else{$g=isset($_POST["databases"])?$_POST["databases"]:[];if(is_string($g))$g=explode("\n",rtrim(str_replace("\r","",$g),"\n"));}$vk=isset($_POST["schemas"])?$_POST["schemas"]:[];$S=array_flip(isset($_POST["tables"])?$_POST["tables"]:[])+array_flip(isset($_POST["data"])?$_POST["data"]:[]);if(count($S)==1)$lf=key($S);elseif(count($vk)==1)$lf=$vk[0];elseif(count($g)==1)$lf=$g[0];else$lf=Admin::get()->getServerName(SERVER,true,"server");$Nd=dump_headers($lf,DB==""||$_GET["ns"]===""||count($S)>1);$Uf=preg_match('~sql~',$_POST["format"]);$rc=$Uf&&$sc&&!$Rl&&DIALECT!="sql";if($Uf){if(!$fc)echo"-- AdminNeo ".VERSION." ".Drivers::get(DRIVER)." ".Connection::get()->getVersion()." dump\n\n";if(DIALECT=="sql"){if(!$fc)echo"SET NAMES utf8;
SET time_zone = '+00:00';
SET foreign_key_checks = 0;
".($sc?"SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO';
":"")."
";Connection::get()->query("SET time_zone = '+00:00'");Connection::get()->query("SET sql_mode = ''");}}$vl=($id?$_POST["db_style"]:"");foreach($g
as$h){if(Connection::get()->selectDatabase($h)){Admin::get()->dumpDatabase($h);if($Uf){if($vl)echo
create_database_sql($h,$vl),use_sql($h,$vl)."\n";$Hi="";if($_POST["types"]){foreach(types()as$r=>$U){$Cc=type_definition($r);$Wh=($Cc["kind"]=='d'?"DOMAIN":"TYPE");if($Cc["definition"])$Hi
.=($vl!='DROP+CREATE'?"DROP $Wh IF EXISTS ".idf_escape($U).";;\n":"")."CREATE $Wh ".idf_escape($U)." $Cc[definition];\n\n";else$Hi
.="-- Could not export type $U\n\n";}}if($_POST["routines"]){foreach(routines()as$K){$A=$K["ROUTINE_NAME"];$ik=$K["ROUTINE_TYPE"];$e=create_routine($ik,["name"=>$A]+routine($K["SPECIFIC_NAME"],$ik));set_utf8mb4($e);$Hi
.=($vl!='DROP+CREATE'?"DROP $ik IF EXISTS ".idf_escape($A).";;\n":"")."$e;\n\n";}}if($_POST["events"]){foreach(get_rows("SHOW EVENTS",null,"-- ")as$K){$e=remove_definer(Connection::get()->getValue("SHOW CREATE EVENT ".idf_escape($K["Name"]),3));set_utf8mb4($e);$Hi
.=($vl!='DROP+CREATE'?"DROP EVENT IF EXISTS ".idf_escape($K["Name"]).";;\n":"")."$e;;\n\n";}}echo($Hi&&DIALECT=='sql'?"DELIMITER ;;\n\n$Hi"."DELIMITER ;\n\n":$Hi);}if($Rl||$sc){foreach(($_GET["ns"]===""?(array)$_POST["schemas"]:(DB!=""||!support("scheme")?[""]:Admin::get()->getSchemas(true)))as$uk){if($uk!="")set_schema($uk);$Ul=table_status('',true);$Nl=array_keys($Ul);$Oc=false;if($rc&&$Nl){$Uj=[];foreach($Nl
as$A){if(!is_view($Ul[$A])&&(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]))){foreach(foreign_keys($A)as$o)$Uj[$A][]=$o["table"];}}$xi=dump_table_order($Nl,$Uj);if($xi)$Nl=$xi;else$Oc=function_exists('AdminNeo\foreign_key_checks_sql');}if($Oc)echo
foreign_key_checks_sql(false)."\n";$mn=[];foreach($Nl
as$A){$R=$Ul[$A];$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));$f=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]));if($Q||$f){$om=null;if($Nd=="tar"){$om=new
TmpFile();ob_start([$om,'write'],1e5);}$hc=($Q||!$id?$Rl:"");Admin::get()->dumpTable($A,$hc,(is_view($R)?2:0));if(is_view($R)&&$Nd!="tar")$mn[]=$A;elseif($f){$l=fields($A);Admin::get()->dumpData($A,$sc,"SELECT *".convert_fields($l,$l)." FROM ".table($A));if($Uf&&!$hc&&$_POST["auto_increment"]&&function_exists('AdminNeo\restart_sequences_sql'))echo"\n".restart_sequences_sql($A);}if($Uf&&$_POST["triggers"]&&$Q&&($zm=trigger_sql($A)))echo"\nDELIMITER ;;\n$zm\nDELIMITER ;\n";if($Nd=="tar"){ob_end_flush();tar_file((DB!=""?"":"$h/")."$A.csv",$om);}elseif($Uf)echo"\n";}}if($Oc)echo
foreign_key_checks_sql(true)."\n";if($Uf&&$Rl&&function_exists('AdminNeo\foreign_keys_sql')){foreach($Ul
as$A=>$R){$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));if($Q&&!is_view($R))echo
foreign_keys_sql($A);}}foreach($mn
as$kn)Admin::get()->dumpTable($kn,$Rl,1);if($Nd=="tar")echo
pack("x512");}}}}if($Uf&&!$fc)echo"-- ".gmdate("Y-m-d H:i:s e")."\n";exit;}$A=DB!=""?h(DB):h(Admin::get()->getServerName(SERVER));page_header(lang(75).": $A",($_GET["export"]!=""?["table"=>$_GET["export"]]:[lang(75)]));echo"<form id='dump-form' action='' method='post'>\n","<table class='box'>\n";$wc=['','USE','DROP+CREATE','CREATE'];$Rl=['','DROP+CREATE','CREATE'];$sc=['','TRUNCATE+INSERT','INSERT'];if(DIALECT=="sql")$sc[]='INSERT+UPDATE';echo"<tr><th>",lang(172),"</th><td>",html_radios("format",$tl+$qc,$O->getParameter("dumpFormat","sql")),"</td></tr>\n";if(DIALECT!="sqlite"){echo"<tr><th id='label-db'>",lang(31),"</th>","<td>",html_select('db_style',$wc,$O->getParameter("dumpDbStyle",DB==""?"CREATE":""),"","label-db"),"<span class='labels'>";if(support("routine"))echo
checkbox("routines",1,$O->getParameter("dumpRoutines",$_GET["dump"]==""?"1":""),lang(186));if(support("event"))echo
checkbox("events",1,$O->getParameter("dumpEvents",$_GET["dump"]==""?"1":""),lang(187));echo"</span></td></tr>";}echo"<tr><th id='label-tables'>",lang(163),"</th><td>",html_select('table_style',$Rl,$O->getParameter("dumpTableStyle","DROP+CREATE"),"","label-tables")," <span class='labels'>",checkbox("auto_increment",1,$O->getParameter("dumpAutoIncrement"),lang(49));if(support("trigger"))echo
checkbox("triggers",1,$O->getParameter("dumpTriggers","1"),lang(183));echo"</span></td></tr>","<tr><th id='label-data'>",lang(188),"</th><td>",html_select("data_style",$sc,$O->getParameter("dumpDataStyle","INSERT"),"","label-data"),"</td></tr>","<tr><th>",lang(189),"</th><td>",html_radios("output",Admin::get()->getDumpOutputs(),$O->getParameter("dumpOutput","file")),"</td></tr>\n","</table>\n",script("initDumpFormats(".json_encode(array_keys($tl)).", ".json_encode(array_keys($qc)).");"),"<p>","<input type='submit' class='button default' value='",lang(75),"'>",input_token(),"</p>\n","<table>\n",script("qsl('table').onclick = dumpClick;");$uj=[];if(DB!=""&&$_GET["ns"]===""){echo"<thead><tr><th>","<label class='block'><input type='checkbox' id='check-schemas' checked class='jsonly' title='".lang(190)."'>".lang(191)."</label>".script("gid('check-schemas').onclick = partial(formCheck, /^schemas\\[/);",""),"</thead>\n";foreach(Admin::get()->getSchemas()as$uk){if(!information_schema(DB,$uk))echo"<tr><td>".checkbox("schemas[]",$uk,true,$uk,"","block")."\n";}}elseif(DB!=""){$ub=($a!=""?"":" checked");echo"<thead><tr>","<th><label class='block'><input type='checkbox' id='check-tables'$ub class='jsonly' title='".lang(190)."'>".lang(9)."</label>".script("gid('check-tables').onclick = partial(formCheck, /^tables\\[/);",""),"<th class='right'><label class='block'>".lang(188)."<input type='checkbox' id='check-data'$ub class='jsonly' title='".lang(190)."'></label>".script("gid('check-data').onclick = partial(formCheck, /^data\\[/);",""),"</thead>\n";$mn="";$Tl=tables_list();foreach($Tl
as$A=>$U){$tj=preg_replace('~_.*~','',$A);$ub=($a==""||$a==(substr($a,-1)=="%"?"$tj%":$A));$xj="<tr><td>".checkbox("tables[]",$A,$ub,$A,"","block");if($U!==null&&!preg_match('~table~i',$U))$mn
.="$xj\n";else
echo"$xj<td class='right'><label class='block'><span id='Rows-".h($A)."'></span>".checkbox("data[]",$A,$ub)."</label>\n";$uj[$tj]++;}echo$mn;if($Tl)echo
script("ajaxSetHtml('".js_escape(ME)."script=db');");}else{$g=Admin::get()->getDatabases();echo"<thead><tr><th>","<label class='block'>".($g?"<input type='checkbox' id='check-databases'".($a==""?" checked":"")." class='jsonly' title='".lang(190)."'>".script("gid('check-databases').onclick = partial(formCheck, /^databases\\[/);",""):"").lang(31)."</label>","</thead>\n";if($g){foreach($g
as$h){if(!information_schema($h)){$tj=preg_replace('~_.*~','',$h);echo"<tr><td>".checkbox("databases[]",$h,$a==""||$a=="$tj%",$h,"","block")."\n";$uj[$tj]++;}}}else
echo"<tr><td><textarea name='databases' rows='10' cols='20'></textarea>";}echo"</table>\n","</form>\n";$Hg=[];foreach($uj
as$u=>$X){if($u!=""&&$X>1)$Hg[]="<a href='".h(ME)."dump=".urlencode("$u%")."'>".icon("check").h($u)."*</a>";}if($Hg)echo"<p class='links'>",implode("",$Hg),"</p>\n";}elseif(isset($_GET["privileges"])){$lm=DB!=""?h(": ".DB):"";page_header(lang(73).$lm,[lang(73)]);echo'<p class="links top-links"><a href="',h(ME),'user=">',icon("user-add"),lang(192),"</a></p>\n";$I=Connection::get()->query("SELECT User, Host FROM mysql.".(DB==""?"user":"db WHERE ".q(DB)." LIKE Db")." ORDER BY Host, User");$He=$I;if(!$I)$I=Connection::get()->query("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', 1) AS User, SUBSTRING_INDEX(CURRENT_USER, '@', -1) AS Host");echo"<form action=''>\n";hidden_fields_get();echo
input_hidden("db",DB);if(!$He)echo
input_hidden("grant");echo"\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr><th>".lang(6)."<th>".lang(5)."<th></thead>\n";while($K=$I->fetchAssoc())echo'<tr><td>'.h($K["User"])."<td>".h($K["Host"]).'<td><a href="'.h(ME.'user='.urlencode($K["User"]).'&host='.urlencode($K["Host"])).'">'.lang(39)."</a>\n";if(!$He||DB!="")echo"<tr><td><input class='input' name='user' autocapitalize='off'>"."<td><input class='input' name='host' value='localhost' autocapitalize='off'>"."<td><input type='submit' class='button' value='".lang(39)."'>\n";echo"</table>\n","</div>\n","</form>\n";}elseif(isset($_GET["sql"])){function
get_unsupported_query($H,$gl){if(DIALECT!="sqlite"||preg_match("~^(?:$gl|;)*+(ATTACH|VACUUM\\b.*\\bINTO)\\b~is",$H,$z)===0)return
null;return
preg_match('~ATTACH~i',isset($z[1])?$z[1]:"")?'ATTACH':'VACUUM INTO';}$O=Admin::get()->getSettings();$Dg=Driver::get()->lineComment();$gl="(?:\\s|\xEF\xBB\xBF|/\\*[\s\S]*?\\*/|(?:$Dg)[^\n]*\n?|--\r?\n)";if($_POST["export"]&&!get_unsupported_query($_POST["query"],$gl)){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers("sql");Admin::get()->dumpTable("","");Admin::get()->dumpData("","table",$_POST["query"]);exit;}restart_session();$df=&get_session("queries");$cf=&$df[DB];if($_POST["clear"]){$cf=[];redirect(remove_from_uri("history"));}stop_session();$T=isset($_GET["import"])?lang(74):lang(41);page_header($T,[$T]);if($_POST){$ye=false;if(!isset($_GET["import"]))$H=$_POST["query"];elseif($_POST["webfile"]){$pf=Admin::get()->getImportFilePath();if($pf){if(file_exists($pf))$ye=fopen($pf,"rb");elseif(file_exists("$pf.gz"))$ye=fopen("compress.zlib://$pf.gz","rb");}$H=$ye?fread($ye,1e6):false;}else$H=get_file("sql_file",true,";");if(is_string($H)){if(($fh=ini_bytes("memory_limit"))!="-1")ini_set("memory_limit",max($fh,strval(2*strlen($H)+memory_get_usage()+8e6)));if($H!=""&&strlen($H)<1e6){$Ej=$H.(preg_match("~;[ \t\r\n]*\$~",$H)?"":";");if(!$cf||first(end($cf))!=$Ej){restart_session();$cf[]=[$Ej,time()];set_session("queries",$df);stop_session();}}$Ec=";";$Fc=1;$Xh=0;$rd=true;$dc=false;$Vb=connect();if($Vb&&DB!=""){$Vb->selectDatabase(DB);if($_GET["ns"]!="")set_schema($_GET["ns"],$Vb);}$Lb=0;$Ad=[];$Pi='[\'"'.(DIALECT=="sql"?'`':(DIALECT=="sqlite"?'`[':(DIALECT=="mssql"?'[':''))).']|/\*|'.$Dg.'|$'.(DIALECT=="pgsql"?'|\$([a-zA-Z]\w*)?\$':'');$sm=microtime(true);$qc=Admin::get()->getDataDumpFormats();unset($qc["sql"]);while($H!=""){if(!$Xh&&preg_match("~^$gl*+DELIMITER\\s+(\\S+)~i",$H,$z)){$Ec=preg_quote($z[1]);$Fc=strlen($z[1]);$ue=Admin::get()->formatSqlCommandQuery(trim($z[0]));if($ue!="")echo"<pre><code class='jush-".DIALECT."'>$ue</code></pre>\n";$H=substr($H,strlen($z[0]));}elseif(!$Xh&&DIALECT=="pgsql"&&preg_match("~^($gl*+COPY\\s+)[^;]+\\s+FROM\\s+stdin;~i",$H,$z)){$Ec="\n\\\\\\.\r?\n";$Fc=3;$dc=true;$Xh=strlen($z[0]);}else{preg_match("($Ec\\s*|$Pi)",$H,$z,PREG_OFFSET_CAPTURE,$Xh);list($we,$G)=$z[0];if(!$we&&$ye&&!feof($ye))$H
.=fread($ye,1e5);else{if(!$we&&rtrim($H)=="")break;$Xh=$G+strlen($we);if($we&&!preg_match("(^$Ec)",$we)){$ib=Driver::get()->hasCStyleEscapes()||(DIALECT=="pgsql"&&($G>0&&strtolower($H[$G-1])=="e"));$cj='(';if($we=='/*')$cj
.='\*/';elseif($we=='[')$cj
.=']';elseif(preg_match("~^(?:$Dg)~",$we))$cj
.="\n";else$cj
.=preg_quote($we).($ib?"|\\\\.":"");$cj
.='|$)s';while(preg_match($cj,$H,$z,PREG_OFFSET_CAPTURE,$Xh)){$mk=$z[0][0];if(!$mk&&$ye&&!feof($ye))$H
.=fread($ye,1e5);else{$Xh=$z[0][1]+strlen($mk);if(!isset($mk[0])||$mk[0]!="\\")break;}}}elseif(DIALECT=="sqlite"&&preg_match("~^(?:$gl|;)*+\$~",substr($H,0,$G+$Fc))){$H=substr($H,$Xh);$Xh=0;}else{$rd=false;$Ej=substr($H,0,$G+$Fc);$Lb++;$xj="<pre id='sql-$Lb'><code class='jush-".DIALECT."'>".Admin::get()->formatSqlCommandQuery(trim($Ej))."</code></pre>\n";if($Om=get_unsupported_query($Ej,$gl)){echo$xj,"<p class='error'>".lang(193,$Om)."\n";$Ad[]=" <a href='#sql-$Lb'>$Lb</a>";if($_POST["error_stops"])break;}else{if(!$_POST["only_errors"]){echo$xj;ob_flush();flush();}$ml=microtime(true);if(Connection::get()->multiQuery($Ej)&&is_object($Vb)&&preg_match("~^$gl*+USE\\b~i",$Ej))$Vb->query($Ej);do{$I=Connection::get()->storeResult();if(Connection::get()->getError()){echo($_POST["only_errors"]?$xj:""),"<p class='error'>",lang(194),(!empty(Connection::get()->getErrno())?" (".Connection::get()->getErrno().")":""),": ",error()."</p>\n";$Ad[]=" <a href='#sql-$Lb'>$Lb</a>";if($_POST["error_stops"])break
2;}else{$im=" <span class='time'>(".format_time($ml).")</span>";$md=(strlen($Ej)<1000?" <a href='".h(ME)."sql=".urlencode(trim($Ej))."'>".icon("edit").lang(39)."</a>":"");$Ij=Connection::get()->getQueryInfo();$za=Connection::get()->getAffectedRows();$wh=Connection::get()->hasMoreResults();$sn=($_POST["only_errors"]||$wh?null:Driver::get()->warnings());$un=($wh?Connection::get()->getWarningsCount():0);$vn="warnings-$Lb";if($sn)$wn="<a href='#$vn' class='toggle'>".lang(40).icon_chevron_down()."</a>";elseif($un)$wn="<span>".lang(40).": $un</span>";else$wn=null;$Id=$_i=null;$Jd="explain-$Lb";$Kd=false;$Ld="export-$Lb";$w=0;if(is_object($I)){if(!$_POST["only_errors"])echo"<div class='table-result'>\n";$w=(int)$_POST["limit"];$_i=print_select_result($I,$Vb,[],$w);if(!$_POST["only_errors"]){echo"<p class='links'>";$Th=$I->getRowsCount();echo($Th?($w&&$Th>$w?lang(195,$w):"").lang(196,$Th):""),$im,$md,$wn;if($Vb&&preg_match("~^($gl|\\()*+SELECT\\b~i",$Ej)&&($Id=explain($Vb,$Ej)))echo"<a href='#$Jd' class='toggle'>Explain".icon_chevron_down()."</a>";$Kd=true;echo"<a href='#$Ld' class='toggle'>".lang(75).icon_chevron_down()."</a>","</p>\n";}}else{if(preg_match("~^$gl*+(CREATE|DROP|ALTER)$gl++(DATABASE|SCHEMA)\\b~i",$Ej)){restart_session();set_session("dbs",null);stop_session();}if(!$_POST["only_errors"]){echo"<p class='message' title='".h($Ij)."'>",lang(197,$za),"$im $md";if($wn)echo", $wn";echo"</p>\n";}}if(!$_POST["only_errors"])echo
script("initToggles(qsl('p'));");if($sn)echo"<div id='$vn' class='hidden'>\n$sn</div>\n";if($Id){echo"<div id='$Jd' class='hidden explain'>\n";print_select_result($Id,$Vb,$_i);echo"</div>\n";}if($Kd){echo"<form id='$Ld' action='' method='post' class='hidden'><p>\n",html_select("format",$qc,$O->getParameter("exportFormat")),html_select("output",Admin::get()->getDumpOutputs(),$O->getParameter("exportOutput"))." ",input_hidden("query",$Ej),input_token()," <input type='submit' class='button' name='export' value='".lang(75)."'>";if(!$w)echo
script("qsl('input').onclick = function (event) { return sqlExport.call(this, event, '".js_escape(ME)."set=export-settings'); };","");echo"</p></form>\n";}if(is_object($I)&&!$_POST["only_errors"])echo"</div>\n";}$ml=microtime(true);}while(Connection::get()->nextResult());}$H=substr($H,$Xh);$Xh=0;if($dc){$Ec=";";$Fc=1;$dc=false;}}}}}if($rd)echo"<p class='message'>".lang(198)."\n";else{$sf=Connection::get()->inTransaction();Driver::get()->rollback();if($sf)echo"<pre><code class='jush-".DIALECT."'>ROLLBACK -- AdminNeo</code></pre>\n";if($_POST["only_errors"]){$ai=$Lb-count($Ad);echo"<p class='".($ai?"message":"error")."'>".lang(199,$Lb-count($Ad))," <span class='time'>(".format_time($sm).")</span>\n";}elseif($Ad&&$Lb>1)echo"<p class='error'>".lang(194).": ".implode("",$Ad)."\n";}}else
echo"<p class='error'>".upload_error($H)."\n";}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";if(!isset($_GET["import"])){$Ej=$_GET["sql"];if($_POST)$Ej=$_POST["query"];elseif($_GET["history"]=="all")$Ej=$cf;elseif($_GET["history"]!="")$Ej=$cf[$_GET["history"]][0];echo"<p>";textarea("query",$Ej,20);echo
script(($_POST?"":"qs('textarea').focus();\n")."gid('form').onsubmit = partial(sqlSubmit, gid('form'), '".js_escape(remove_from_uri("sql|limit|error_stops|only_errors|history"))."');"),"</p>","<p><input type='submit' class='button default' value='".lang(200)."' title='Ctrl+Enter'>",lang(201).": <input type='number' name='limit' class='input size' value='".h($_POST?$_POST["limit"]:$_GET["limit"])."'>\n";}else{echo"<div class='field-sets'>\n","<fieldset><legend>".lang(202)."</legend><div class='fieldset-content'>";$Pe=(extension_loaded("zlib")?"[.gz]":"");if(ini_bool("file_uploads"))echo"SQL$Pe (&lt; ".ini_get("upload_max_filesize")."B): <input type='file' name='sql_file[]' multiple>","<input type='submit' class='button default' value='".lang(200)."'>",file_upload_form_script("form","sql_file[]");else
echo
lang(203);echo"</div></fieldset>\n";$pf=Admin::get()->getImportFilePath();if($pf)echo"<fieldset><legend>".lang(204)."</legend><div class='fieldset-content'>",lang(205,"<code>".h($pf)."$Pe</code>")," <input type='submit' class='button default' name='webfile' value='".lang(206)."'>","</div></fieldset>\n";echo"</div>\n","<p>";}echo
checkbox("error_stops",1,($_POST?$_POST["error_stops"]:(isset($_GET["error_stops"])?$_GET["error_stops"]:true)),lang(207)),checkbox("only_errors",1,($_POST?$_POST["only_errors"]:isset($_GET["import"])||$_GET["only_errors"]),lang(208)),input_token(),"</p>\n";if(!isset($_GET["import"]))Admin::get()->printAfterSqlCommand();if(!isset($_GET["import"])&&$cf){echo"<div class='field-sets'>\n";print_fieldset_start("history",lang(209),"history",$_GET["history"]!="");for($X=end($cf);$X;$X=prev($cf)){$u=key($cf);list($Ej,$im,$qd)=$X;echo" <pre><code class='jush-".DIALECT."'>",truncate_utf8(preg_replace('~\s+~',' ',ltrim(preg_replace("~^(?:$Dg).*~m",'',$Ej)))),"</code></pre>",'<p class="links">',"<a href='".h(ME."sql=&history=$u")."'>".icon("edit").lang(39)."</a>"," <span class='time' title='".@date('Y-m-d',$im)."'>".@date("H:i:s",$im).($qd?" ($qd)":"")."</span>","</p>";}echo"<p><input type='submit' class='button' name='clear' value='".lang(210)."'>\n","<a href='",h(ME."sql=&history=all")."' class='button light'>",icon("edit"),lang(211),"</a></p>\n";print_fieldset_end("history");echo"</div>\n";}echo"</form>\n";}elseif(isset($_GET["edit"])){$a=$_GET["edit"];$l=fields($a);$Z=(isset($_GET["select"])?($_POST["check"]&&count($_POST["check"])==1?where_check($_POST["check"][0],$l):""):where($_GET,$l));$Qm=(isset($_GET["select"])?$_POST["edit"]:$Z);foreach($l
as$A=>$k){if((!$Qm&&!isset($k["privileges"]["insert"]))||Admin::get()->getFieldName($k)=="")unset($l[$A]);}if($_POST&&!isset($_GET["select"])){$y=$_POST["referer"];if($_POST["insert"])$y=($Qm?null:$_SERVER["REQUEST_URI"]);elseif(!preg_match('~^.+&select=.+$~',$y))$y=ME."select=".urlencode($a);$t=indexes($a);$Im=unique_array(isset($_GET["where"])?$_GET["where"]:[],$t);$Jj="\nWHERE $Z";if(isset($_POST["delete"]))queries_redirect($y,lang(212),(bool)Driver::get()->delete($a,$Jj,$Im?0:1));else{$Sk=[];foreach($l
as$A=>$k){$X=process_input($k);if($X!==false&&$X!==null)$Sk[idf_escape($A)]=$X;}if($Qm){if(!$Sk)redirect($y);queries_redirect($y,lang(213),(bool)Driver::get()->update($a,$Sk,$Jj,$Im?0:1));if(is_ajax()){page_headers();page_messages();exit;}}else{$I=Driver::get()->insert($a,$Sk);$tg=($I?last_id($I):0);queries_redirect($y,lang(214,($tg?" $tg":"")),(bool)$I);}}}$K=null;if($Z){$M=[];foreach($l
as$A=>$k){if(isset($k["privileges"]["select"])){$La=($_POST["clone"]&&$k["auto_increment"]?"''":convert_field($k));$M[]=($La?"$La AS ":"").idf_escape($A);}}$K=[];if(!support("table"))$M=["*"];if($M){$I=Driver::get()->select($a,$M,[$Z],$M,[],(isset($_GET["select"])?2:1));if(!$I)Admin::get()->addError(error());else{$K=$I->fetchAssoc();if(!$K)$K=false;}if(isset($_GET["select"])&&(!$K||$I->fetchAssoc()))$K=null;}}if(!support("table")&&!$l){if(!$Z){$I=Driver::get()->select($a,["*"],[],["*"]);$K=($I?$I->fetchAssoc():false);if(!$K)$K=[Driver::get()->primary=>""];}if($K){foreach($K
as$u=>$X){if(!$Z)$K[$u]=null;$l[$u]=["field"=>$u,"null"=>($u!=Driver::get()->primary),"auto_increment"=>($u==Driver::get()->primary)];}}}if(isset($_POST["save"])?$_POST["save"]:false){$qj=[];foreach((isset($_POST["fields"])?$_POST["fields"]:[])as$u=>$X)$qj[bracket_escape($u,true)]=$X;$K=$qj+($K?:[]);}if($_POST["edit"]){$od=array_filter($l,function($k){return!(isset($k["generated"])?$k["generated"]:null);});}else$od=$l;edit_form($a,$od,$K,$Qm);}elseif(isset($_GET["create"])){$a=$_GET["create"];$Ti=Driver::get()->getPartitionBy();$Xi=$Ti?Driver::get()->getPartitionsInfo($a):[];$Rj=referencable_primary($a);$qe=[];foreach($Rj
as$Ml=>$k)$qe[str_replace("`","``",$Ml)."`".str_replace("`","``",$k["field"])]=$Ml;$Ci=[];$R=[];if($a!=""){$Ci=fields($a);$R=table_status1($a);if(count($R)<2)Admin::get()->addError(lang(79));}$K=$_POST;$K["Comment"]=normalize_newlines($K["Comment"]);$K["fields"]=(array)$K["fields"];if($K["auto_increment_col"])$K["fields"][$K["auto_increment_col"]]["auto_increment"]=true;if($_POST&&!Admin::get()->getErrors())Admin::get()->getSettings()->updateParameter("commentsOpened",isset($_POST["comments"])?$_POST["comments"]:null);if($_POST&&!process_fields($K["fields"])&&!Admin::get()->getErrors()){if($_POST["drop"])queries_redirect(substr(ME,0,-1),lang(215),drop_tables([$a]));else{$l=[];$Ea=[];$Vm=false;$oe=[];$Bi=reset($Ci);$Aa=" FIRST";foreach($K["fields"]as$u=>$k){$o=$qe[$k["type"]];$Bm=($o!==null?$Rj[$o]:$k);if($k["field"]!=""){if(!$k["generated"])$k["default"]=null;$Cj=process_field($k,$Bm);$Ea[]=[$k["orig"],$Cj,$Aa];if(!$Bi||$Cj!==process_field($Bi,$Bi)){$l[]=[$k["orig"],$Cj,$Aa];if($k["orig"]!=""||$Aa)$Vm=true;}if($o!==null)$oe[idf_escape($k["field"])]=($a!=""&&DIALECT!="sqlite"?"ADD":" ").format_foreign_key(['table'=>$qe[$k["type"]],'source'=>[$k["field"]],'target'=>[$Bm["field"]],'on_delete'=>$k["on_delete"],]);$Aa=" AFTER ".idf_escape($k["field"]);}elseif($k["orig"]!=""){$Vm=true;$l[]=[$k["orig"]];}if($k["orig"]!=""){$Bi=next($Ci);if(!$Bi)$Aa="";}}$Vi=[];if(in_array($K["partition_by"],$Ti)){foreach($K
as$u=>$X){if(preg_match('~^partition~',$u))$Vi[$u]=$X;}foreach($Vi["partition_names"]as$u=>$A){if($A===""){unset($Vi["partition_names"][$u]);unset($Vi["partition_values"][$u]);}}$Vi["partition_names"]=array_values($Vi["partition_names"]);$Vi["partition_values"]=array_values($Vi["partition_values"]);if($Vi==$Xi)$Vi=[];}elseif(str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned"))$Vi=null;$hh=lang(216);if($a==""){cookie("neo_engine",isset($K["Engine"])?$K["Engine"]:"");$hh=lang(217);}$A=trim($K["name"]);$y=ME.(support("table")?"table=":"select=").urlencode($A);$I=alter_table($a,$A,(DIALECT=="sqlite"&&($Vm||$oe)?$Ea:$l),$oe,($K["Comment"]!=$R["Comment"]?$K["Comment"]:null),($K["Engine"]&&$K["Engine"]!=$R["Engine"]?$K["Engine"]:""),($K["Collation"]&&$K["Collation"]!=$R["Collation"]?$K["Collation"]:""),($K["Auto_increment"]!=""?number($K["Auto_increment"]):""),$Vi);if($I&&!Queries::$queries)redirect($y);queries_redirect($y,$hh,$I);}}if($a!="")page_header(lang(36).": ".h($a),["table"=>$a,lang(36)]);else
page_header(lang(78),[lang(78)]);if(!$_POST){$Em=Driver::get()->getTypes();$K=["Engine"=>$_COOKIE["neo_engine"],"fields"=>[["field"=>"","type"=>(isset($Em["int"])?"int":(isset($Em["integer"])?"integer":"")),"on_update"=>""]],"partition_names"=>[""],];if($a!=""){$K=$R;$K["name"]=$a;$K["fields"]=[];if(!$_GET["auto_increment"])$K["Auto_increment"]="";foreach($Ci
as$k){$k["generated"]=$k["generated"]?:(isset($k["default"])?"DEFAULT":"");$K["fields"][]=$k;}if($Ti){$K+=$Xi;$K["partition_names"][]="";$K["partition_values"][]="";}}}$eg=[];if($K["Collation"])$eg[$K["Collation"]]=true;foreach($K["fields"]as$k){if($k["collation"])$eg[$k["collation"]]=true;}$Db=Admin::get()->getCollations(array_keys($eg));$wd=Driver::get()->engines();foreach($wd
as$vd){if(!strcasecmp($vd,$K["Engine"])){$K["Engine"]=$vd;break;}}$Ug=max_input_vars(12,20);if($Ug){$Ze=(count($K["fields"])>$Ug?"":" hidden");echo"<p".($Ze?" id='max-fields' data-columns='$Ug'":"")." class='error$Ze'>".max_input_vars_error()."\n";}echo"<form action='' method='post' id='form'>\n";if(support("columns")||$a==""){echo"<p>",lang(218),": ","<input class='input' name='name' data-maxlength='64' value='",h($K["name"]),"' autocapitalize='off'",(($a==""&&!$_POST)?" autofocus":""),">";if($wd)echo" ",html_select("Engine",[""=>"(".lang(219).")"]+$wd,$K["Engine"]),help_script_command("value",true);if($Db&&!preg_match("~sqlite|mssql~",DIALECT))echo" ",html_select("Collation",[""=>"(".lang(92).")"]+$Db,$K["Collation"]);echo" <input type='submit' class='button default' value='",lang(116),"'>","</p>";}if(support("columns")&&($a==""||!Driver::get()->isPartition($a))){echo"<div class='scrollable'>\n","<table id='edit-fields' class='nowrap'>\n";edit_fields($K["fields"],$Db,"TABLE",$qe);echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>",lang(49),": ","<input type='number' class='input size' name='Auto_increment' size='6' value='",h($K["Auto_increment"]),"'>";$Pb=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Nb=$Pb?"":"hidden";if(support("comment")){echo
checkbox("comments",1,$Pb,lang(48),"editingCommentsClick(this);","jsonly")," ";if(preg_match('~\n~',$K["Comment"]))echo"<textarea name='Comment' rows='2' cols='20'",($Nb?" class='$Nb'":""),">",h($K["Comment"]),"</textarea>";else
echo"<input name='Comment' value='",h($K["Comment"]),"' data-maxlength='",(Connection::get()->isMinVersion("5.5")?2048:60),"' class='input $Nb'>";}echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(116),"'>";}elseif($a!="")echo"<p>";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$a)),"</p>\n";if($Ti&&(DIALECT=="sql"||$a=="")){echo"<div class='field-sets'>\n";$Ui=preg_match('~RANGE|LIST~',$K["partition_by"]);print_fieldset_start("partition",lang(221),"split",(bool)$K["partition_by"]);echo"<p>",html_select("partition_by",array_merge([""],$Ti),$K["partition_by"]),help_script_command("value.replace(/./, 'PARTITION BY \$&')",true),script("qsl('select').onchange = partitionByChange;"),"(<input class='input' name='partition' value='",h($K["partition"]),"'>) ",lang(50),": ","<input type='number' name='partitions' class='input size ",($Ui||!$K["partition_by"]?"hidden":""),"' value='",h($K["partitions"]),"'>","</p>\n","<table id='partition-table'",($Ui?"":" class='hidden'"),">\n","<thead><tr><th>",lang(222),"</th><th>",lang(52),"</th></tr></thead>\n";foreach($K["partition_names"]as$u=>$X){echo"<tr>","<td><input class='input' name='partition_names[]' value='",h($X),"' autocapitalize='off'>";if($u==count($K["partition_names"])-1)echo
script("qsl('input').oninput = partitionNameChange;");echo"</td>","<td><input class='input' name='partition_values[]' value='",h(isset($K["partition_values"][$u])?$K["partition_values"][$u]:""),"'></td>","</tr>\n";}echo"</table>\n","</p>\n";print_fieldset_end("partition");echo"</div>\n";}echo
input_token(),"</form>\n";}elseif(isset($_GET["indexes"])){$a=$_GET["indexes"];$wf=["PRIMARY","UNIQUE","INDEX"];$R=table_status1($a,true);$uf=Driver::get()->getIndexAlgorithms($R);$d=Connection::get();$Qg=$d->isMariaDB();if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($Qg?"10.0.5":"5.6")?'|InnoDB':'').'~i',$R["Engine"]))$wf[]="FULLTEXT";if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($Qg?"10.2.2":"5.7")?'|InnoDB':'').'~i',$R["Engine"]))$wf[]="SPATIAL";if($Qg&&$d->isMinVersion("11.7")&&preg_match('~MyISAM|InnoDB~i',$R["Engine"]))$wf[]="VECTOR";$t=indexes($a);$l=fields($a);$wj=[];if(DIALECT=="mongo"){$wj=$t["_id_"];unset($wf[0]);unset($t["_id_"]);}$K=$_POST;if($K){$O=Admin::get()->getSettings();if($O->getParameter("indexOptions")!==null)$O->updateParameter("indexOptions",null);}if($_POST&&!$_POST["add"]&&!$_POST["drop_col"]){$Ga=[];foreach($K["indexes"]as$s){$A=$s["name"];if(in_array($s["type"],$wf)){$c=[];$Ag=[];$Jc=[];$ni=[];$tf=$uf?(in_array($s["algorithm"],$uf)?$s["algorithm"]:first($uf)):"";$vf=(support("partial_indexes")?$s["partial"]:"");$Sk=[];ksort($s["columns"]);foreach($s["columns"]as$u=>$b){if($b!=""){$v=isset($s["lengths"][$u])?$s["lengths"][$u]:null;$Hc=isset($s["descs"][$u])?$s["descs"][$u]:null;$mi=isset($s["opclasses"][$u])?$s["opclasses"][$u]:null;$Sk[]=($l[$b]?idf_escape($b):$b).($v?"(".(+$v).")":"").($mi!=""?" ".idf_escape($mi):"").($Hc?" DESC":"");$c[]=$b;$Ag[]=($v?:null);$Jc[]=$Hc;$ni[]="$mi";}}$Hd=$t[$A];if($Hd){ksort($Hd["columns"]);ksort($Hd["lengths"]);ksort($Hd["descs"]);if($s["type"]==$Hd["type"]&&array_values($Hd["columns"])===$c&&(!$Hd["lengths"]||array_values($Hd["lengths"])===$Ag)&&array_values($Hd["descs"])===$Jc&&(!$Hd["opclasses"]||array_values($Hd["opclasses"])===$ni)&&(!$uf||$Hd["algorithm"]===$tf)&&$Hd["partial"]==$vf){unset($t[$A]);continue;}}if($c)$Ga[]=[$s["type"],$A,$Sk,$tf,$vf];}}foreach($t
as$A=>$Hd)$Ga[]=[$Hd["type"],$A,"DROP"];if(!$Ga)redirect(ME."table=".urlencode($a));queries_redirect(ME."table=".urlencode($a),lang(223),alter_indexes($a,$Ga));}page_header(lang(176).": ".h($a),["table"=>$a,lang(176)]);$be=array_keys($l);if($_POST["add"]){foreach($K["indexes"]as$u=>$s){if($s["columns"][count($s["columns"])]!="")$K["indexes"][$u]["columns"][]="";}$s=end($K["indexes"]);if($s["type"]||array_filter($s["columns"],'strlen'))$K["indexes"][]=["columns"=>[1=>""]];}if(!$K){foreach($t
as$u=>$s){$t[$u]["name"]=$u;$t[$u]["columns"][]="";}$t[]=["columns"=>[1=>""]];$K["indexes"]=$t;}$Ag=(DIALECT=="sql"||DIALECT=="mssql");$ni=Driver::get()->getIndexOpclasses();if($_POST)$Wk=$_POST["options"];else{$Wk=false;foreach($t
as$s){if(array_filter(isset($s["lengths"])?$s["lengths"]:[])||array_filter(isset($s["descs"])?$s["descs"]:[])||array_filter(isset($s["opclasses"])?$s["opclasses"]:[])||(isset($s["partial"])?$s["partial"]:"")!=""){$Wk=true;break;}}}echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th id='label-type'>",lang(224),"</th>";$si="class='idxopts".($Wk?"":" hidden")."'";if(count($uf)>1)echo"<th id='label-method' $si>",lang(225),doc_link(['sql'=>'create-index.html#create-index-storage-engine-index-types','mariadb'=>'ha-and-performance/optimization-and-tuning/optimization-and-indexes/storage-engine-index-types',]),"</th>";echo"<th><input type='submit' hidden>",lang(53).($Ag?"<span ".str_replace("class='","class='note ",$si)."> (".lang(54).")</span>":"");if($Ag||support("descidx"))echo
checkbox("options",1,$Wk,lang(98),"indexOptionsShow(this.checked)","jsonly")."\n";echo"</th>","<th id='label-name'>",lang(226),"</th>";if(support("partial_indexes"))echo"<th id='label-condition' $si>",lang(55),"</th>";echo"<th>","<button name='add[0]' value='1' title='",lang(100),"' class='button light hidden'>",icon_solo("add"),"</button>","</th>","</tr></thead>\n";if($wj){echo"<tr><td>PRIMARY<td>";foreach($wj["columns"]as$b)echo
select_input(" disabled",array_combine($be,$be),$b),"<label><input type='checkbox' disabled>".lang(63)."</label> ";echo"<td><td>\n";}$Zf=1;foreach($K["indexes"]as$s){if(!$_POST["drop_col"]||$Zf!=key($_POST["drop_col"])){echo"<tr><td>",html_select("indexes[$Zf][type]",[-1=>""]+$wf,$s["type"],($Zf==count($K["indexes"])?"indexesAddRow.call(this);":""),"label-type"),"</td>";if(count($uf)>1)echo"<td $si>",html_select("indexes[$Zf][algorithm]",array_merge([""],$uf),$s['algorithm'],"label-method"),"</td>";echo"<td>";ksort($s["columns"]);$q=1;foreach($s["columns"]as$u=>$b){echo"<span>".select_input(" name='indexes[$Zf][columns][$q]' title='".lang(44)."'",($l&&($b==""||$l[$b])?array_combine($be,$be):[]),$b,"partial(indexesChangeColumn, '".js_escape(DIALECT=="sql"?"":$_GET["indexes"]."_")."')"),"<span $si>";if($Ag)echo"<input type='number' name='indexes[$Zf][lengths][$q]' class='input size' value='".(h(isset($s["lengths"][$u])?$s["lengths"][$u]:"")),"' title='".lang(97),"'>";if($ni){$mi=isset($s["opclasses"][$u])?$s["opclasses"][$u]:"";echo
html_select("indexes[$Zf][opclasses][$q]",[""=>"(".lang(227).")"]+array_combine($ni,$ni)+($mi!=""?[$mi=>$mi]:[]),$mi),'';}if(support("descidx"))echo
checkbox("indexes[$Zf][descs][$q]",1,isset($s["descs"][$u])?$s["descs"][$u]:false,lang(63));echo"<br></span></span>";$q++;}echo"</td>","<td><input name='indexes[$Zf][name]' value='",h($s["name"]),"' class='input' autocapitalize='off' aria-labelledby='label-name'></td>\n";if(support("partial_indexes"))echo"<td $si><input name='indexes[$Zf][partial]' value='".h($s["partial"])."' autocapitalize='off' aria-labelledby='label-condition'>\n";echo"<td>","<button name='drop_col[$Zf]' value='1' title='",lang(59),"' class='button light'>",icon_solo("remove"),"</button>",script("qsl('button').onclick = onRemoveIndexRowClick;"),"</td>\n";}$Zf++;}echo"</table>\n","</div>\n","<p>","<input type='submit' class='button default' value='",lang(116),"'>",input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["database"])){$K=$_POST;if($_POST&&!isset($_POST["add_x"])){$A=trim($K["name"]);if($_POST["drop"]){$_GET["db"]="";queries_redirect(remove_from_uri("db|database"),lang(228),drop_databases([DB]));}elseif(DB!==$A){if(DB!=""){$_GET["db"]=$A;queries_redirect(preg_replace('~\bdb=[^&]*&~','',ME)."db=".urlencode($A),lang(229),rename_database($A,$K["collation"]));}else{$g=explode("\n",str_replace("\r","",$A));$xl=true;$sg="";foreach($g
as$h){if(count($g)==1||$h!=""){if(!create_database($h,$K["collation"]))$xl=false;$sg=$h;}}restart_session();set_session("dbs",null);queries_redirect(ME."db=".urlencode($sg),lang(230),$xl);}}else{if(!$K["collation"])redirect(substr(ME,0,-1));query_redirect("ALTER DATABASE ".idf_escape($A).(preg_match('~^[a-z0-9_]+$~i',$K["collation"])?" COLLATE $K[collation]":""),substr(ME,0,-1),lang(231));}}if(DB!="")page_header(lang(70).": ".h(DB),[lang(70)]);else
page_header(lang(76),[lang(76)]);$A=DB;if($_POST)$A=$K["name"];elseif(DB!="")$K["collation"]=db_collation(DB,collations());elseif(DIALECT=="sql"){foreach(get_vals("SHOW GRANTS")as$He){if(preg_match('~ ON (`(([^\\\\`]|``|\\\\.)*)%`\.\*)?~',$He,$z)&&$z[1]){$A=stripcslashes(idf_unescape("`$z[2]`"));break;}}}$Db=Admin::get()->getCollations($K["collation"]?[$K["collation"]]:[]);echo"<form action='' method='post'>\n","<p>";if($_POST["add_x"]||strpos($A,"\n"))echo"<textarea id='name' name='name' rows='10' cols='40'>",h($A),"</textarea><br>\n";else
echo"<input class='input' name='name' id='name' value='",h($A),"' data-maxlength='64' autocapitalize='off' autofocus>\n";if($Db)echo
html_select("collation",[""=>"(".lang(92).")"]+$Db,$K["collation"]),doc_link(['sql'=>"charset-charsets.html",'mariadb'=>"reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations",]),"\n";echo"<input type='submit' class='button default' value='",lang(116),"'>\n";if(DB!="")echo"<input type='submit' class='button' name='drop' value='".lang(167)."'>".confirm(lang(220,DB))."\n";elseif(!$_POST["add_x"]&&$_GET["db"]=="")echo"<button name='add_x' value='1' title='",lang(100),"' class='button light'>",icon_solo("add"),"</button>\n";echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["call"])){$oa=$_GET["name"]?:$_GET["call"];page_header(lang(232).": ".h($oa),[lang(232)]);$ik=routine($_GET["call"],(isset($_GET["callf"])?"FUNCTION":"PROCEDURE"));$qf=[];$Hi=[];foreach($ik["fields"]as$q=>$k){if(substr($k["inout"],-3)=="OUT"&&DIALECT=='sql')$Hi[$q]="@".idf_escape($k["field"])." AS ".idf_escape($k["field"]);if(!$k["inout"]||substr($k["inout"],0,2)=="IN")$qf[]=$q;}if($_POST){$kb=[];foreach($ik["fields"]as$u=>$k){$X="";if(in_array($u,$qf)){$X=process_input($k);if($X===false)$X="''";if(isset($Hi[$u]))Connection::get()->query("SET @".idf_escape($k["field"])." = $X");}if(isset($Hi[$u]))$kb[]="@".idf_escape($k["field"]);elseif(in_array($u,$qf))$kb[]=$X;}$H=(isset($_GET["callf"])?"SELECT ":"CALL ").($ik["returns"]&&$ik["returns"]["type"]=="record"?"* FROM ":"").table($oa)."(".implode(", ",$kb).")";$ml=microtime(true);$I=Connection::get()->multiQuery($H);$za=Connection::get()->getAffectedRows();echo
Admin::get()->formatSelectQuery($H,$ml,!$I);if(!$I)echo"<p class='error'>".error()."\n";else{$Vb=connect();if($Vb)$Vb->selectDatabase(DB);$Nh=false;do{$I=Connection::get()->storeResult();if($Nh&&Connection::get()->getError())echo"<p class='error'>".error()."\n";elseif(is_object($I))print_select_result($I,$Vb);else
echo"<p class='message'>".lang(233,$za)." <span class='time'>".@date("H:i:s")."</span>\n";}while($Nh=Connection::get()->nextResult());if($Hi)print_select_result(Connection::get()->query("SELECT ".implode(", ",$Hi)));}}echo"<form action='' method='post'>\n";if($qf){echo"<table class='box'>\n";foreach($qf
as$u){$k=$ik["fields"][$u];$A=$k["field"];echo"<tr><th>".Admin::get()->getFieldName($k);$Y=isset($_POST["fields"][$A])?$_POST["fields"][$A]:"";if($Y!=""){if($k["type"]=="set")$Y=implode(",",$Y);}input($k,$Y,(string)(isset($_POST["function"][$A])?$_POST["function"][$A]:""));echo"\n";}echo"</table>\n";}echo"<p>\n","<input type='submit' class='button' value='",lang(232),"'>\n",input_token(),"</p>\n","</form>\n";$Mb=$ik["comment"];if($Mb!==null&&$Mb!==""){$Mb=h(trim($ik["comment"],"\n"));if(preg_match('~^ +~',$Mb,$_)){preg_match_all("~^($_[0]|$)~m",$Mb,$Fg);if(count($Fg[0])==substr_count($Mb,"\n"))$Mb=preg_replace("~^($_[0])~m","",$Mb);}$Mb=preg_replace('~(^|[^\n]\n)(Description|Parameters|Example)\n~',"$1\n<strong>$2</strong>\n",$Mb);echo"<pre class='comment'>$Mb</pre>\n";}}elseif(isset($_GET["foreign"])){$a=$_GET["foreign"];$A=$_GET["name"];$K=$_POST;if($_POST&&!$_POST["add"]&&!$_POST["change"]&&!$_POST["change-js"]){if(!$_POST["drop"]){$K["source"]=array_filter($K["source"],'strlen');ksort($K["source"]);$Yl=[];foreach($K["source"]as$u=>$X)$Yl[$u]=$K["target"][$u];$K["target"]=$Yl;}if(DIALECT=="sqlite")$I=recreate_table($a,$a,[],[],[" $A"=>($K["drop"]?"":" ".format_foreign_key($K))]);else{$Ga="ALTER TABLE ".table($a);$I=($A==""||queries("$Ga DROP ".(DIALECT=="sql"?"FOREIGN KEY ":"CONSTRAINT ").idf_escape($A)));if(!$K["drop"])$I=queries("$Ga ADD".format_foreign_key($K));}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(234):($A!=""?lang(235):lang(236))),(bool)$I);if(!$K["drop"])Admin::get()->addError(lang(237));}if($A!="")page_header(lang(238).": ".h($A),["table"=>$a,lang(238)]);else
page_header(lang(180).": ".h($a),["table"=>$a,lang(180)]);if($_POST){ksort($K["source"]);if($_POST["change"]||$_POST["change-js"])$K["target"]=[];else$K["source"][]="";}elseif($A!=""){$qe=foreign_keys($a);$K=$qe[$A];$K["source"][]="";}else{$K["table"]=$a;$K["source"]=[""];}echo"<form action='' method='post'>\n";$el=array_keys(fields($a));if($K["db"]!="")Connection::get()->selectDatabase($K["db"]);if($K["ns"]!=""){$Di=get_schema();set_schema($K["ns"]);}$Qj=array_keys(array_filter(table_status('',true),'AdminNeo\fk_support'));$Yl=$Qj?array_keys(fields(in_array($K["table"],$Qj)?$K["table"]:reset($Qj))):[];$ii="this.form['change-js'].value = '1'; this.form.submit();";echo"<p>","<span id='label-table'>",lang(239),":</span> ",html_select("table",$Qj,$K["table"],$ii,"label-table");if(DIALECT!="sqlite"){$xc=[];foreach(Admin::get()->getDatabases()as$h){if(!information_schema($h))$xc[]=$h;}echo"<span id='label-db'>",lang(240),":</span> ",html_select("db",$xc,$K["db"]!=""?$K["db"]:$_GET["db"],$ii,"label-db");}echo
input_hidden("change-js"),"<noscript><input type='submit' class='button' name='change' value='",lang(241),"'></noscript>","</p>\n","<table>","<thead><tr><th id='label-source'>",lang(177),"<th id='label-target'>",lang(178),"</thead>\n";$Zf=0;foreach($K["source"]as$u=>$X){echo"<tr>","<td>".html_select("source[".(+$u)."]",[-1=>""]+$el,$X,($Zf==count($K["source"])-1?"foreignAddRow.call(this);":""),"label-source"),"<td>".html_select("target[".(+$u)."]",$Yl,isset($K["target"][$u])?$K["target"][$u]:null,"","label-target");$Zf++;}echo"</table>\n","<noscript><p><input type='submit' class='button' name='add' value='",lang(242),"'></p></noscript>","<p>\n","<span id='label-delete'>".lang(94),":</span> ",html_select("on_delete",[-1=>""]+Driver::get()->getOnActions(),$K["on_delete"],"","label-delete"),"<span id='label-update'>".lang(93),":</span> ",html_select("on_update",[-1=>""]+Driver::get()->getOnActions(),$K["on_update"],"","label-update");if(DRIVER=='pgsql')echo
html_select("deferrable",['NOT DEFERRABLE','DEFERRABLE','DEFERRABLE INITIALLY DEFERRED'],$K["deferrable"]);echo
doc_link(['sql'=>"innodb-foreign-key-constraints.html",'mariadb'=>"architecture/server-constraints/foreign-key-constraints",]),"</p>\n<p>","<input type='submit' class='button default' value='",lang(116),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["view"])){$a=$_GET["view"];$K=$_POST;$Ei="VIEW";if(DIALECT=="pgsql"&&$a!=""){$ol=table_status1($a);$Ei=strtoupper($ol["Engine"]);}if($_POST){$A=trim($K["name"]);$La=" AS\n$K[select]";$y=ME."table=".urlencode($A);$hh=lang(243);$U=($_POST["materialized"]?"MATERIALIZED VIEW":"VIEW");if(!$_POST["drop"]&&$a==$A&&DIALECT!="sqlite"&&$U=="VIEW"&&$Ei=="VIEW")query_redirect((DIALECT=="mssql"?"ALTER":"CREATE OR REPLACE")." VIEW ".table($A).$La,$y,$hh);else{$am="adminneo_".uniqid();drop_create("DROP $Ei ".table($a),"CREATE $U ".table($A).$La,"DROP $U ".table($A),"CREATE $U ".table($am).$La,"DROP $U ".table($am),($_POST["drop"]?substr(ME,0,-1):$y),lang(244),$hh,lang(245),$a,$A);}}if(!$_POST&&$a!=""){$K=view($a);$K["name"]=$a;$K["materialized"]=($Ei!="VIEW");if($j=error())Admin::get()->addError($j);}if($a!="")page_header(lang(37).": ".h($a),["table"=>$a,lang(37)]);else
page_header(lang(246),[lang(246)]);echo"<form action='' method='post'>\n","<p>",lang(226),":","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>\n";if(support("materializedview"))echo
checkbox("materialized",1,$K["materialized"],lang(168));echo"</p>\n<p>";textarea("select",$K["select"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(116),"'>\n";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>\n",confirm(lang(220,$a));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["event"])){$ea=$_GET["event"];$Jf=["YEAR","QUARTER","MONTH","DAY","HOUR","MINUTE","WEEK","SECOND","YEAR_MONTH","DAY_HOUR","DAY_MINUTE","DAY_SECOND","HOUR_MINUTE","HOUR_SECOND","MINUTE_SECOND"];$pl=["ENABLED"=>"ENABLE","DISABLED"=>"DISABLE","SLAVESIDE_DISABLED"=>"DISABLE ON SLAVE"];$K=$_POST;if($_POST){if($_POST["drop"])query_redirect("DROP EVENT ".idf_escape($ea),substr(ME,0,-1),lang(247));elseif(in_array($K["INTERVAL_FIELD"],$Jf)&&isset($pl[$K["STATUS"]])){$tk="\nON SCHEDULE ".($K["INTERVAL_VALUE"]?"EVERY ".q($K["INTERVAL_VALUE"])." $K[INTERVAL_FIELD]".($K["STARTS"]?" STARTS ".q($K["STARTS"]):"").($K["ENDS"]?" ENDS ".q($K["ENDS"]):""):"AT ".q($K["STARTS"]))." ON COMPLETION".($K["ON_COMPLETION"]?"":" NOT")." PRESERVE";queries_redirect(substr(ME,0,-1),($ea!=""?lang(248):lang(249)),(bool)queries(($ea!=""?"ALTER EVENT ".idf_escape($ea).$tk.($ea!=$K["EVENT_NAME"]?"\nRENAME TO ".idf_escape($K["EVENT_NAME"]):""):"CREATE EVENT ".idf_escape($K["EVENT_NAME"]).$tk)."\n".$pl[$K["STATUS"]]." COMMENT ".q($K["EVENT_COMMENT"]).rtrim(" DO\n$K[EVENT_DEFINITION]",";").";"));}}if($ea!="")page_header(lang(250).": ".h($ea),[lang(250)]);else
page_header(lang(251),[lang(251)]);if(!$K&&$ea!=""){$L=get_rows("SELECT * FROM information_schema.EVENTS WHERE EVENT_SCHEMA = ".q(DB)." AND EVENT_NAME = ".q($ea));$K=reset($L);}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(226),"</th><td>","<input class='input' name='EVENT_NAME' value='",h($K["EVENT_NAME"]),"' data-maxlength='64' autocapitalize='off'>","</td></tr>\n","<tr><th title='datetime'>",lang(252),"</th><td>","<input class='input' name='STARTS' value='",h("$K[EXECUTE_AT]$K[STARTS]"),"'>","</td></tr>\n","<tr><th title='datetime'>",lang(253),"</th><td>","<input class='input' name='ENDS' value='",h($K["ENDS"]),"'>","</td></tr>\n","<tr><th>",lang(254),"</th><td>","<input type='number' name='INTERVAL_VALUE' value='",h($K["INTERVAL_VALUE"]),"' class='input size'> ",html_select("INTERVAL_FIELD",$Jf,$K["INTERVAL_FIELD"]),"</td></tr>\n","<tr><th>",lang(159),"</th><td>",html_select("STATUS",$pl,$K["STATUS"]),"</td></tr>\n","<tr><th>",lang(48),"</th><td>","<input class='input' name='EVENT_COMMENT' value='",h($K["EVENT_COMMENT"]),"' data-maxlength='64'>","</td></tr>\n","<tr><th></th><td>",checkbox("ON_COMPLETION","PRESERVE",$K["ON_COMPLETION"]=="PRESERVE",lang(255)),"</td></tr>\n","</table>\n","<p>";textarea("EVENT_DEFINITION",$K["EVENT_DEFINITION"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(116),"'>";if($ea!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$ea));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["procedure"])){$oa=($_GET["name"]?:$_GET["procedure"]);$ik=(isset($_GET["function"])?"FUNCTION":"PROCEDURE");$K=$_POST;$K["fields"]=(array)$K["fields"];if($_POST&&!process_fields($K["fields"])){foreach($K["fields"]as$u=>$k){if($k["field"]=="")unset($K["fields"][$u]);}$di=routine_id($oa,routine($_GET["procedure"],$ik));$Kh=routine_id($K["name"],$K);$e=create_routine($ik,$K);$y=substr(ME,0,-1);$hh=lang(256);if(!$_POST["drop"]&&$di==$Kh&&(DIALECT!="sql"||Connection::get()->isMariaDB()))query_redirect(substr_replace($e,' OR REPLACE',6,0),$y,$hh);else{$am="adminneo_".uniqid();drop_create("DROP $ik $di",$e,"DROP $ik $Kh",create_routine($ik,["name"=>$am]+$K),"DROP $ik ".routine_id($am,$K),$y,lang(257),$hh,lang(258),$oa,$K["name"]);}}if($oa!=""){$T=isset($_GET["function"])?lang(259):lang(260);page_header($T.": ".h($oa),[$T]);}else{$T=isset($_GET["function"])?lang(261):lang(262);page_header($T,[$T]);}if(!$_POST){if($oa=="")$K["language"]="sql";else{$K=routine($_GET["procedure"],$ik);$K["name"]=$oa;}}$qb=get_vals("SHOW CHARACTER SET");sort($qb);$jk=routine_languages();echo"<form action='' method='post' id='form'>\n","<p>",lang(226),": ","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>";if($jk)echo"<span id='label-language'>",lang(10),":</span> ",html_select("language",$jk,$K["language"],"","label-language");echo"<input type='submit' class='button default' value='",lang(116),"'>","</p>\n","<div class='scrollable'>\n","<table class='nowrap' id='edit-fields'>\n";edit_fields($K["fields"],$qb,$ik);if(isset($_GET["function"])){echo"<tbody><tr>";if(support("move_col"))echo"<th></th>";echo"<th>",lang(263),"</th>";edit_type("returns",(array)$K["returns"],$qb,[],(DIALECT=="pgsql"?["void","trigger"]:[]));echo"<td></td>","</tr></tbody>\n";}echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>";textarea("definition",$K["definition"],20);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(116),"'>";if($oa!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$oa));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["check"])){$a=$_GET["check"];$A=$_GET["name"];$K=$_POST;if($K){if(DIALECT=="sqlite")$xl=recreate_table($a,$a,[],[],[],"",[],"$A",($K["drop"]?"":$K["clause"]));else{$xl=($A==""||queries("ALTER TABLE ".table($a)." DROP CONSTRAINT ".idf_escape($A)));if(!$K["drop"])$xl=(bool)queries("ALTER TABLE ".table($a)." ADD".($K["name"]!=""?" CONSTRAINT ".idf_escape($K["name"]):"")." CHECK ($K[clause])");}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(264):($A!=""?lang(265):lang(266))),$xl);}if($A!="")page_header(lang(267).": ".h($A),["table"=>$a,lang(267)]);else
page_header(lang(182).": ".h($a),["table"=>$a,lang(182)]);if(!$K){$vb=Driver::get()->checkConstraints($a);$K=["name"=>$A,"clause"=>$vb[$A]];}echo"<form action='' method='post'>\n","<p>";if(DIALECT!="sqlite")echo
lang(226).': <input name="name" value="'.h($K["name"]).'" class="input" data-maxlength="64" autocapitalize="off"> ';echo
doc_link(['sql'=>"create-table-check-constraints.html",'mariadb'=>"reference/sql-statements/data-definition/constraint",],"?"),"</p>\n<p>";textarea("clause",$K["clause"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(116),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["trigger"])){$a=$_GET["trigger"];$A=isset($_GET["name"])?$_GET["name"]:"";$ym=trigger_options();$K=trigger($A,$a)+["Trigger"=>$a."_bi"];if($_POST){if(in_array($_POST["Timing"],$ym["Timing"])&&in_array($_POST["Event"],$ym["Event"])&&in_array($_POST["Type"],$ym["Type"])){$gi=" ON ".table($a);$cd="DROP TRIGGER ".idf_escape($A).(DIALECT=="pgsql"?$gi:"");$y=ME."table=".urlencode($a);if($_POST["drop"])query_redirect($cd,$y,lang(268));else{if($A!="")queries($cd);queries_redirect($y,($A!=""?lang(269):lang(270)),(bool)queries(create_trigger($gi,$_POST)));if($A!="")queries(create_trigger($gi,$K+["Type"=>reset($ym["Type"])]));}}$K=$_POST;}if($A!="")page_header(lang(271).": ".h($A),["table"=>$a,lang(271)]);else
page_header(lang(184).": ".h($a),["table"=>$a,lang(184)]);echo"<form action='' method='post' id='form'>\n","<table class='box box-light'>\n","<tr><th id='label-time'>",lang(272),"</th><td>",html_select("Timing",$ym["Timing"],$K["Timing"],"triggerChange(/^".js_escape_re($a)."_[ba][iud]$/, '".js_escape($a)."', this.form);","label-time"),"</td></tr>\n","<tr><th id='label-event'>",lang(273),"</th><td>",html_select("Event",$ym["Event"],$K["Event"],"this.form['Timing'].onchange();","label-event");if(in_array("UPDATE OF",$ym["Event"]))echo" <input name='Of' value='".h($K["Of"])."' class='input hidden'>";echo"</td></tr>\n","<tr><th id='label-type'>",lang(45),"</th><td>",html_select("Type",$ym["Type"],$K["Type"],"","label-type"),"</td></tr>\n","</table>\n","<p>",lang(226),"<input class='input' name='Trigger' value='",h($K["Trigger"]),"' data-maxlength='64' autocapitalize='off'>","</p>\n",script("gid('form')['Timing'].onchange();"),"<p>";textarea("Statement",$K["Statement"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(116),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>",confirm(lang(220,$A));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["user"])){$qa=$_GET["user"];$_j=[""=>["All privileges"=>""]];foreach(get_rows("SHOW PRIVILEGES")as$K){foreach(explode(",",($K["Privilege"]=="Grant option"?"":$K["Context"]))as$Zb)$_j[$Zb=="File access on server"?"Server Admin":$Zb][$K["Privilege"]]=$K["Comment"];}unset($_j["Server Admin"]["Usage"]);foreach($_j["Tables"]as$u=>$X)unset($_j["Databases"][$u]);$Jh=[];if($_POST){foreach($_POST["objects"]as$u=>$X)$Jh[$X]=(array)$Jh[$X]+(array)$_POST["grants"][$u];}$Je=[];if(isset($_GET["host"])&&($I=Connection::get()->query("SHOW GRANTS FOR ".q($qa)."@".q($_GET["host"])))){while($K=$I->fetchRow()){if(preg_match('~GRANT (.*) ON (.*) TO ~',$K[0],$z)&&preg_match_all('~ *([^(,]*[^ ,(])( *\([^)]+\))?~',$z[1],$_,PREG_SET_ORDER)){foreach($_
as$X){if($X[1]!="USAGE")$Je["$z[2]$X[2]"][$X[1]]=true;if(preg_match('~ WITH GRANT OPTION~',$K[0]))$Je["$z[2]$X[2]"]["GRANT OPTION"]=true;}}}}$hj=!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8");if($_POST){$fi=(isset($_GET["host"])?q($qa)."@".q($_GET["host"]):"''");if($_POST["drop"])query_redirect("DROP USER $fi",ME."privileges=",lang(274));else{$Mh=q($_POST["user"])."@".q($_POST["host"]);$Zi=$_POST["pass"];$jc=false;$I=true;if($fi!=$Mh){$jc=(bool)queries("CREATE USER $Mh IDENTIFIED BY ".($_POST["hashed"]?"PASSWORD ":"").q($Zi));$I=$jc;}elseif($Zi!="")$I=(bool)queries("SET PASSWORD FOR $Mh = ".($hj||$_POST["hashed"]?q($Zi):"PASSWORD(".q($Zi).")"));if($I){$fk=[];foreach($Jh
as$Wh=>$He){if(isset($_GET["grant"]))$He=array_filter($He);$He=array_keys($He);if(isset($_GET["grant"]))$fk=array_diff(array_keys(array_filter($Jh[$Wh],'strlen')),$He);elseif($fi==$Mh){$ci=array_keys((array)$Je[$Wh]);$fk=array_diff($ci,$He);$He=array_diff($He,$ci);unset($Je[$Wh]);}if(preg_match('~^(.+)\s*(\(.*\))?$~U',$Wh,$z)&&(!grant(false,$fk,$z[2],$z[1],$Mh)||!grant(true,$He,$z[2],$z[1],$Mh))){$I=false;break;}}}if($I&&isset($_GET["host"])){if($fi!=$Mh)queries("DROP USER $fi");elseif(!isset($_GET["grant"])){foreach($Je
as$Wh=>$fk){if(preg_match('~^(.+)(\(.*\))?$~U',$Wh,$z))grant(false,array_keys($fk),$z[2],$z[1],$Mh);}}}if($I&&!Queries::$queries)redirect(ME."privileges=");queries_redirect(ME."privileges=",(isset($_GET["host"])?lang(275):lang(276)),$I);if($jc)Connection::get()->query("DROP USER $Mh");}}$T=isset($_GET["host"])?lang(6).": ".h("$qa@$_GET[host]"):lang(192);$lm=isset($_GET["host"])?h($qa):lang(192);page_header($T,["privileges"=>['',lang(73)],$lm]);if($_POST){$K=$_POST;$Je=$Jh;}else{$K=$_GET+["host"=>Connection::get()->getValue("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', -1)")];if($Je)$Je[".*"]=[];elseif(DB!="")$Je[idf_escape(addcslashes(DB,"%_\\")).".*"]=[];else$Je["*.* "]=[];}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(5),"</th>","<td><input class='input' name='host' data-maxlength='60' value='",h($K["host"]),"' autocapitalize='off'></td>\n","<tr><th>",lang(6),"</th>","<td><input class='input' name='user' data-maxlength='80' value='",h($K["user"]),"' autocapitalize='off'></td>\n",'<tr><th>',lang(30),"</th>","<td><input class='input' name='pass' id='pass' value='",h($K["pass"]),"' autocomplete='new-password'>";if(!$hj)echo
checkbox("hashed",1,$K["hashed"],lang(277),"typePassword(this.form['pass'], this.checked);");echo"</td>\n";if(!$K["hashed"])echo
script("typePassword(gid('pass'));");echo"</table>\n","<div class='scrollable'><table class='checkable'>\n","<thead><tr><th colspan='2'>".lang(73).doc_link(['sql'=>"grant.html#priv_level","mariadb"=>"reference/sql-statements/account-management-sql-statements/grant#privilege-levels"])."</th>";$q=0;foreach($Je
as$Wh=>$He){echo"<th>";if($Wh=="*.*")echo"*.*",input_hidden("objects[$q]","*.*");else
echo"<input class='input' name='objects[$q]' value='".h(trim($Wh))."' size='10' autocapitalize='off'>";echo"</th>";$q++;}echo"</tr></thead>\n";foreach([""=>"","Server Admin"=>lang(5),"Databases"=>lang(31),"Tables"=>lang(9),"Procedures"=>lang(278),]as$Zb=>$Hc){foreach((array)$_j[$Zb]as$zj=>$Mb){echo"<tr>";if($Hc)echo"<td>$Hc</td>";echo"<td".(!$Hc?" colspan='2'":"").' lang="en" title="'.h($Mb).'">'.h($zj)."</td>";$q=0;foreach($Je
as$Wh=>$He){$A="'grants[$q][".h(strtoupper($zj))."]'";$Y=$He[strtoupper($zj)];$Dj=strpos($Wh,"@")!==false;$Ih=$Wh==".*";$Ca=$zj=="All privileges";$Ie=$zj=="Grant option";if($Wh=="*.*"&&$zj=="Proxy")echo"<td></td>";elseif($Dj&&$zj!="Proxy"&&!$Ie)echo"<td></td>";elseif($Zb=="Server Admin"&&$Wh!=(isset($Je["*.*"])?"*.*":".*")&&!(($Dj||$Ih)&&$zj=="Proxy"))echo"<td></td>";elseif(isset($_GET["grant"]))echo"<td><select name=$A>"."<option></option>"."<option value='1'".($Y?" selected":"").">".lang(279)."</option>"."<option value='0'".($Y=="0"?" selected":"").">".lang(280)."</option>"."</select></td>";else{echo"<td class='center'><label class='block'>","<input type='checkbox' name=$A value='1'".($Y?" checked":"").($Ca?" id='grants-$q-all'":(!$Ie?" class='grants-$q'":"")).">";if($Ca)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheckAll('.grants-$q'); };");elseif(!$Ie)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheck('grants-$q-all'); };");echo"</label>";}$q++;}echo"</tr>";}}echo"</table></div>\n","<p>","<input type='submit' class='button default' value='",lang(116),"'>\n";if(isset($_GET["host"]))echo"<input type='submit' class='button' name='drop' value='",lang(167),"'>\n",confirm(lang(220,"$qa@$_GET[host]"));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["processlist"])){if(support("kill")){if($_POST){$kg=0;foreach((array)$_POST["kill"]as$X){if(kill_process($X))$kg++;}queries_redirect(ME."processlist=",lang(281,$kg),$kg||!$_POST["kill"]);}}page_header(lang(157),[lang(157)]);echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap checkable'>\n";$q=-1;foreach(process_list()as$q=>$K){if(!$q){echo"<thead><tr lang='en'>".(support("kill")?"<th>":"");foreach($K
as$u=>$X)echo"<th>$u".doc_link(['sql'=>"show-processlist.html#processlist_".strtolower($u),'mariadb'=>"reference/sql-statements/administrative-sql-statements/show/show-processlist",]);echo"</thead>\n","<tbody>\n";}echo"<tr>".(support("kill")?"<td>".checkbox("kill[]",$K[DIALECT=="sql"?"Id":"pid"],0):"");foreach($K
as$u=>$X)echo"<td>".($X!=""&&((DIALECT=="sql"&&$u=="Info"&&preg_match("~Query|Killed~",$K["Command"]))||(DIALECT=="pgsql"&&$u=="query")||(DIALECT=="oracle"&&$u=="sql_text"))?"<code class='jush-".DIALECT."'>".truncate_utf8($X,100).'</code> <a href="'.h(ME.($K["db"]!=""?"db=".urlencode($K["db"])."&":"")."sql=".urlencode($X)).'">'.icon("edit").lang(282).'</a>':h($X));echo"\n";}if($q>=0)echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});");echo"</table>\n","</div>\n","<p>";if(support("kill"))echo($q+1)."/".lang(283,max_connections()),"<p><input type='submit' class='button' value='".lang(284)."'>\n";echo
input_token(),"</p>\n","</form>\n",script("tableCheck();");}elseif(isset($_GET["select"])){$a=$_GET["select"];$R=table_status1($a);$t=indexes($a);$l=fields($a);$qe=column_foreign_keys($a);$Yh=$R["Oid"];$gk=[];$c=[];$yk=[];$vi=[];$em=null;foreach($l
as$u=>$k){$A=Admin::get()->getFieldName($k);$Dh=html_entity_decode(strip_tags($A),ENT_QUOTES);if(isset($k["privileges"]["select"])&&$A!=""){$c[$u]=$Dh;if(is_shortable($k))$em=Admin::get()->processSelectionLength();}if(isset($k["privileges"]["where"])&&$A!="")$yk[$u]=$Dh;if(isset($k["privileges"]["order"])&&$A!="")$vi[$u]=$Dh;$gk+=$k["privileges"];}list($M,$Ke)=Admin::get()->processSelectionColumns($c,$t);$M=array_unique($M);$Ke=array_unique($Ke);$Sf=count($Ke)<count($M);$Z=Admin::get()->processSelectionSearch($l,$t);$D=Admin::get()->processSelectionOrder($l,$t);$w=Admin::get()->processSelectionLimit();if($_GET["modify"]&&!Admin::get()->isDataEditAllowed())redirect(ME."select=".urlencode($a));if($_GET["val"]&&is_ajax()){header("Content-Type: text/plain; charset=utf-8");foreach($_GET["val"]as$Jm=>$K){$La=convert_field($l[key($K)]);$M=[$La?:idf_escape(key($K))];$Z[]=where_check($Jm,$l);$J=Driver::get()->select($a,$M,$Z,$M);if(is_object($J))echo
first($J->fetchRow());}exit;}$wj=$Mm=[];foreach($t
as$s){if($s["type"]=="PRIMARY"){$wj=array_flip($s["columns"]);$Mm=($M?$wj:[]);foreach($Mm
as$u=>$X){if(in_array(idf_escape($u),$M))unset($Mm[$u]);}break;}}if($Yh&&!$wj){$wj=$Mm=[$Yh=>0];$t[]=["type"=>"PRIMARY","columns"=>[$Yh]];}$O=Admin::get()->getSettings();if($_POST){$yn=$Z;if(!$_POST["all"]&&is_array($_POST["check"])){$vb=[];foreach($_POST["check"]as$rb)$vb[]=where_check($rb,$l);$yn[]="((".implode(") OR (",$vb)."))";}$yn=($yn?"\nWHERE ".implode(" AND ",$yn):"");if($_POST["export"]){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers($a);Admin::get()->dumpTable($a,"");$ze=($M?implode(", ",$M):"*").convert_fields($c,$l,$M)."\nFROM ".table($a);$Ne=($Ke&&$Sf?"\nGROUP BY ".implode(", ",$Ke):"").($D?"\nORDER BY ".implode(", ",$D):"");if(!is_array($_POST["check"])||$wj)$H="SELECT $ze$yn$Ne";else{$Gm=[];foreach($_POST["check"]as$X)$Gm[]="(SELECT".limit($ze,"\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$l).$Ne,1).")";$H=implode(" UNION ALL ",$Gm);}Admin::get()->dumpData($a,"table",$H);exit;}if($_POST["save"]||$_POST["delete"]){$I=true;$za=0;$Sk=[];if(!$_POST["delete"]){$Gk=array_keys($_POST["fields"]+$_POST["function"]);foreach($Gk
as$A){$X=process_input($l[$A]);if($X!==null&&($_POST["clone"]||$X!==false))$Sk[idf_escape($A)]=($X!==false?$X:idf_escape($A));}}if($_POST["delete"]||$Sk){if($_POST["clone"])$H="INTO ".table($a)." (".implode(", ",array_keys($Sk)).")\nSELECT ".implode(", ",$Sk)."\nFROM ".table($a);if($_POST["all"]||($wj&&is_array($_POST["check"]))||$Sf){$I=($_POST["delete"]?Driver::get()->delete($a,$yn):($_POST["clone"]?queries("INSERT $H$yn".Driver::get()->getInsertReturningSql($a)):Driver::get()->update($a,$Sk,$yn)));$za=Connection::get()->getAffectedRows();if(is_object($I))$za+=$I->getRowsCount();}else{foreach((array)$_POST["check"]as$X){$xn="\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$l);$I=($_POST["delete"]?Driver::get()->delete($a,$xn,1):($_POST["clone"]?queries("INSERT".limit1($a,$H,$xn)):Driver::get()->update($a,$Sk,$xn,1)));if(!$I)break;$za+=Connection::get()->getAffectedRows();}}}$hh=lang(285,$za);if($_POST["clone"]&&$I&&$za==1){$tg=last_id($I);if($tg)$hh=lang(214," $tg");}queries_redirect(remove_from_uri($_POST["all"]&&$_POST["delete"]?"page":""),$hh,(bool)$I);if(!$_POST["delete"]){$od=array_filter($l,function($k){return!(isset($k["generated"])?$k["generated"]:null);});edit_form($a,$od,(array)$_POST["fields"],!$_POST["clone"]);page_footer();exit;}}elseif(!$_POST["import"]){if(!$_POST["val"])Admin::get()->addError(lang(286));else{$xl=true;$za=0;foreach($_POST["val"]as$Jm=>$K){$Sk=[];foreach($K
as$u=>$X){$u=bracket_escape($u,true);if(isset($l[$u]["full_type"]))Admin::get()->detectJson($l[$u]["full_type"],$X,false);$Sk[idf_escape($u)]=(preg_match('~char|text~',$l[$u]["type"])||$X!=""?Admin::get()->processFieldInput($l[$u],$X):"NULL");}$xl=(bool)Driver::get()->update($a,$Sk," WHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($Jm,$l),($Sf||$wj?0:1)," ");if(!$xl)break;$za+=Connection::get()->getAffectedRows();}queries_redirect(remove_from_uri(),lang(285,$za),$xl);}}elseif(!is_string($m=get_file("csv_file",true)))Admin::get()->addError(upload_error($m));elseif(!preg_match('~~u',$m))Admin::get()->addError(lang(287));else{$O->updateParameter("exportFormat",$_POST["import_format"]);$Hb=array_keys($l);preg_match_all('~(?>"[^"]*"|[^"\r\n]+)+~',$m,$_);$za=count($_[0]);Driver::get()->begin();$Hk=($_POST["import_format"]=="csv;"?";":($_POST["import_format"]=="tsv"?"\t":","));$L=[];foreach($_[0]as$u=>$X){preg_match_all("~((?>\"[^\"]*\")+|[^$Hk]*)$Hk~",$X.$Hk,$Sg);if(!$u&&!array_diff($Sg[1],$Hb)){$Hb=$Sg[1];$za--;}else{$Sk=[];foreach($Sg[1]as$q=>$Ab)$Sk[idf_escape($Hb[$q])]=($Ab==""&&$l[$Hb[$q]]["null"]?"NULL":q(preg_match('~^".*"$~s',$Ab)?str_replace('""','"',substr($Ab,1,-1)):$Ab));$L[]=$Sk;}}$xl=!$L||Driver::get()->insertUpdate($a,$L,$wj);if($xl)Driver::get()->commit();queries_redirect(remove_from_uri("page"),lang(288,$za),$xl);Driver::get()->rollback();}}$Ml=Admin::get()->getTableName($R);if(is_ajax()){page_headers();ob_start();}else
page_header(lang(56).": $Ml",[$Ml]);$Ef=null;if(isset($gk["insert"])||!support("table")){$Ef=[];foreach((array)$_GET["where"]as$X){if(isset($qe[$X["col"]])&&count($qe[$X["col"]])==1&&($X["op"]=="="||(!$X["op"]&&(is_array($X["val"])||!preg_match('~[_%]~',$X["val"])))))$Ef["preset"."[".bracket_escape($X["col"])."]"]=$X["val"];}}Admin::get()->printTableMenu($R,$Ef);if(!$c&&support("table"))echo"<p class='error'>".lang(289).($l||error()==""?".":": ".error())."\n";else{echo"<form id='form' action=''>\n","<div hidden>";hidden_fields_get();if(DB!=""){echo
input_hidden("db",DB);if(isset($_GET["ns"]))echo
input_hidden("ns",$_GET["ns"]);}echo
input_hidden("select",$a),"<input type='submit' class='button' value='".lang(56)."'>","</div>\n","<div class='field-sets'>\n";Admin::get()->printSelectionColumns($M,$c);Admin::get()->printSelectionSearch($Z,$yk,$t);Admin::get()->printSelectionOrder($D,$vi,$t);Admin::get()->printSelectionLimit($w);Admin::get()->printSelectionLength($em);Admin::get()->printSelectionAction($t);echo"</div>\n</form>\n";foreach((array)$_GET["where"]as$X){if($X["op"]=="SQL"&&!in_array(isset($_SERVER["HTTP_SEC_FETCH_SITE"])?$_SERVER["HTTP_SEC_FETCH_SITE"]:"",["","same-origin"])){echo"<p class='error'>".lang(151)." ".lang(152)."\n";page_footer();exit;}}$E=isset($_GET["page"])?$_GET["page"]:null;if($E=="last"){$xe=Connection::get()->getValue(count_rows($a,$Z,$Sf,$Ke));$E=$w?(int)floor(max(0,intval($xe)-1)/$w):0;}else{$xe=false;$E=(int)$E;}$zk=$M;$Le=$Ke;if(!$zk){$zk[]="*";$bc=convert_fields($c,$l,$M);if($bc)$zk[]=substr($bc,2);}foreach($M
as$u=>$X){$k=$l[idf_unescape($X)];if($k&&($La=convert_field($k)))$zk[$u]="$La AS $X";}if(DIALECT=="pgsql"||DIALECT=="mssql"){foreach((array)$_GET["columns"]as$u=>$X){if(isset($zk[$u])&&$X["fun"])$zk[$u].=" AS ".idf_escape(apply_sql_function($X["fun"],($X["col"]!=""?$X["col"]:"*")));}}if(!$Sf&&$Mm){foreach($Mm
as$u=>$X){$zk[]=idf_escape($u);if($Le)$Le[]=idf_escape($u);}}$I=Driver::get()->select($a,$zk,$Z,$Le,$D,$w,$E,true);if(!is_object($I))echo"<p class='error'>".(error()?:lang(123))."\n";else{if(DIALECT=="mssql"&&$E)$I->seek($w*$E);$L=[];while($K=$I->fetchAssoc()){if($E&&DIALECT=="oracle")unset($K["RNUM"]);$L[]=$K;}if($_GET["modify"]&&$L){$ah=max_input_vars(count($L[0])+1,20);echo($ah&&count($L)>$ah?"<p class='error'>".max_input_vars_error()."\n":"");}echo"<form id='selection_form' action='' method='post' enctype='multipart/form-data'>\n","<div class='table-footer-parent'>\n";if($_GET["page"]!="last"&&$w&&$Ke&&$Sf&&DIALECT=="sql")$xe=Connection::get()->getValue(" SELECT FOUND_ROWS()");$pd=false;if(!$L)echo"<p class='message'>".lang(90)."\n";else{$Va=Admin::get()->getBackwardKeys($a,$Ml);echo"<div class='scrollable'>\n","<table id='table' class='nowrap checkable'>\n","<thead><tr>";if($Ke||!$M){echo"<th class='actions'><input type='checkbox' id='all-page' class='jsonly' title='".lang(290)."'>".script("gid('all-page').onclick = partial(formCheck, /^check/);","");if(Admin::get()->isDataEditAllowed())echo" <a href='",h($_GET["modify"]?remove_from_uri("modify"):$_SERVER["REQUEST_URI"]."&modify=1")."' title='",lang(291),"'>",icon_solo("edit-all"),"</a>";}$Fh=[];$Vh=[];$Ce=[];reset($M);$Lj=1;foreach($L[0]as$u=>$X){if(!isset($Mm[$u])){$Bk=key($M);$X=isset($_GET["columns"][$Bk])?$_GET["columns"][$Bk]:[];$k=$l[$M?($X?$X["col"]:current($M)):$u];$A=($k?Admin::get()->getFieldName($k,$Lj):(isset($X["fun"])?"*":h($u)));if($A!=""){$Lj++;$Fh[$u]=$A;$Vh[$u]=($k&&preg_match(number_type(),$k["type"])&&!preg_match('~\[~',$k["full_type"]))||preg_match('~^(CHAR_LENGTH|ROUND|FLOOR|CEIL|UNIX_TIMESTAMP|TIME_TO_SEC|COUNT|SUM)\(~',(string)current($M));$b=idf_escape($u);$if=remove_from_uri('(order|desc)[^=]*|page').'&order%5B0%5D='.urlencode($u);$Hc="&desc%5B0%5D=1";$ui=isset($D[0])?$D[0]:"";$bl=preg_replace('~ DESC( NULLS LAST)?$~','',$ui);$dl=($bl==$b||$bl==$u);echo"<th id='th[".h(bracket_escape($u))."]'".($Vh[$u]?" class='number'":"").($dl?" aria-sort='".($bl==$ui?"ascending":"descending")."'":"").">";$Be=apply_sql_function(isset($X["fun"])?$X["fun"]:null,$A);$cl=isset($k["privileges"]["order"])||(isset($X["fun"])?$X["fun"]:null);if($cl)echo'<a href="',h($if.($dl&&$bl==$ui?$Hc:'')),'">',"$Be</a>";else
echo$Be;echo"<span class='column'>";if($cl)echo"<a href='".h($if.$Hc)."' title='".lang(63)."' class='button light'>",icon_solo("arrow-down"),"</a>";if(!isset($X["fun"])&&isset($k["privileges"]["where"]))echo"<a href='#fieldset-search' title='".lang(60)."' class='button light jsonly'>",icon_solo("search"),"</a>",script("qsl('a').onclick = partial(selectSearch, '".js_escape($u)."');");echo"</span>";}$Ce[$u]=isset($X["fun"])?$X["fun"]:null;next($M);}}$Ag=[];if($_GET["modify"]){foreach($L
as$K){foreach($K
as$u=>$X)$Ag[$u]=max($Ag[$u],min(40,strlen(utf8_decode($X))));}}if($Va)echo"<th>".lang(19)."</th>";echo"</thead>\n","<tbody>\n";if(is_ajax())ob_end_clean();foreach(Admin::get()->fillForeignDescriptions($L,$qe)as$Bh=>$K){$Im=unique_array($L[$Bh],$t);if(!$Im){$Im=[];reset($M);foreach($L[$Bh]as$u=>$X){if(!preg_match('~^(COUNT|AVG|GROUP_CONCAT|MAX|MIN|SUM)\(~',current($M)))$Im[$u]=$X;next($M);}}$Jm="";foreach($Im
as$u=>$X){$k=isset($l[$u])?$l[$u]:null;$Rf=$k&&is_blob($k);if((DIALECT=="sql"||DIALECT=="pgsql")&&$k&&($Rf||preg_match('~char|text|enum|set~',$k["type"]))&&strlen($X)>64){$u=(strpos($u,'(')?$u:idf_escape($u));$u="MD5(".($Rf||DIALECT!='sql'||preg_match("~^utf8~",isset($k["collation"])?$k["collation"]:"")?$u:"CONVERT($u USING ".charset(Connection::get()).")").")";$X=md5($Rf?(string)Connection::get()->formatValue($X,$k):$X);}$Jm
.="&".($X!==null?urlencode("where[".bracket_escape($u)."]")."=".urlencode($X===false?"f":$X):"null%5B%5D=".urlencode($u));}echo"<tr>";if($Ke||!$M){echo"<td class='actions'>",checkbox("check[]",substr($Jm,1),in_array(substr($Jm,1),(array)$_POST["check"]));if(!$Sf&&Admin::get()->isDataEditAllowed())echo" <a href='",h(ME."edit=".urlencode($a).$Jm),"' class='edit' title='",lang(39),"'>",icon_solo("edit"),"</a>";}reset($M);foreach($K
as$u=>$X){if(isset($Fh[$u])){$b=current($M);$k=isset($l[$u])?$l[$u]:null;$x="";if($k&&is_blob($k)&&$X!="")$x=ME.'download='.urlencode($a).'&field='.urlencode($u).$Jm;if(!$x&&$X!==null){foreach((array)$qe[$u]as$o){if(count($qe[$u])==1||end($o["source"])==$u){$x="";foreach($o["source"]as$q=>$el)$x
.=where_link($q,$o["target"][$q],$L[$Bh][$el]);$x=($o["db"]!=""?preg_replace('~([?&]db=)[^&]+~','\1'.urlencode($o["db"]),ME):ME).'select='.urlencode($o["table"]).$x;if($o["ns"])$x=preg_replace('~([?&]ns=)[^&]+~','\1'.urlencode($o["ns"]),$x);if(count($o["source"])==1)break;}}}if($b=="COUNT(*)"){$x=ME."select=".urlencode($a);$q=0;foreach((array)$_GET["where"]as$W){if(!array_key_exists($W["col"],$Im))$x
.=where_link($q++,$W["col"],$W["val"],$W["op"]);}foreach($Im
as$cg=>$W)$x
.=where_link($q++,$cg,$W);}$Sh=$X===null;$jf=select_value($X,$x,$k,$em);$Cd=bracket_escape($u);$r=h("val[$Jm][$Cd]");$rj=isset($_POST["val"][$Jm][$Cd])?$_POST["val"][$Jm][$Cd]:null;$Qm=isset($k["privileges"]["update"])?$k["privileges"]["update"]:false;$nd=!is_array($X)&&!($k&&is_blob($k))&&is_utf8((string)$X)&&$L[$Bh][$u]==$X&&!$Ce[$u]&&!(isset($k["generated"])?$k["generated"]:false);$U=($b&&preg_match('~^(AVG|MIN|MAX)\((.+)\)~',$b,$_)?$l[idf_unescape($_[2])]["type"]:(isset($k["type"])?$k["type"]:null));$vh=$U=="money"||($b&&preg_match('~^SUM\((.+)\)~',$b,$_)&&$l[idf_unescape($_[1])]["type"])=="money";$cm=$U&&preg_match('~text|json|lob~',$U);$zb=$Vh[$u]&&($Sh||is_numeric(strip_tags($jf))||$vh)?"class='number'":"";echo"<td id='$r' $zb";if(($_GET["modify"]&&$nd&&!$Sh)||$rj!==null){$pd=true;$Qe=h($rj!==null?$rj:$X);echo" data-editing='true'>".($cm?"<textarea name='$r' cols='30' rows='".(substr_count($X,"\n")+1)."'>$Qe</textarea>":"<input class='input' name='$r' value='$Qe' size='$Ag[$u]'>");}else{$Pg=strpos($jf,"<i>…</i>");if($Qm)echo" data-text='".($Pg?2:($cm?1:0))."'".($nd?"":" data-warning='".lang(292)."'");echo">$jf";}}next($M);}if($Va){echo"<td>";Admin::get()->printBackwardKeys($Va,$L[$Bh]);echo"</td>";}echo"</tr>\n";}if(is_ajax())exit;echo"</tbody>\n",script("mixin(qs('#table tbody'), {onclick: event => tableClick(event, false, ".(Admin::get()->isDataEditAllowed()?"true":"false")."), ondblclick: event => tableClick(event, true), onkeydown: onEditingKeydown});"),"</table>\n",script("initToggles(gid('table'));"),"</div>\n";}if(!is_ajax()){if($L||$E){$Ed=true;if($_GET["page"]!="last"){if(!$w||(count($L)<$w&&($L||!$E)))$xe=($E?$E*$w:0)+count($L);elseif(DIALECT!="sql"||!$Sf){$xe=($Sf?false:found_rows($R,$Z));if($xe<max(1e4,2*($E+1)*$w))$xe=first(slow_query(count_rows($a,$Z,$Sf,$Ke)));elseif(DIALECT=='sql'||DIALECT=='pgsql')$Ed=false;}}$Li=($w&&($xe===false||$xe>$w||$E));if($Li){if(($xe===false?count($L)+1:$xe-$E*$w)>$w)echo'<p class="links">','<a href="',h(remove_from_uri("page")."&page=".($E+1)),'" class="loadmore">',icon("expand"),lang(293),'</a>',script("qsl('a').onclick = partial(loadNextPage, $w, '".js_escape(lang(294))."');","");echo"\n";}echo"<div class='table-footer'><div class='field-sets'>\n";if($Li){$Yg=($xe===false?$E+(count($L)>=$w?2:1):(int)floor(($xe-1)/$w));$Yc="<li>…</li>";echo"<fieldset><legend>".lang(295)."</legend>";if(DIALECT!="simpledb"){echo"<div id='fieldset-pagination' class='fieldset-content'><ul class='pagination'>",pagination(0,$E);if($E>5)echo$Yc;for($q=max(1,$E-4);$q<min($Yg,$E+5);$q++)echo
pagination($q,$E);if($Yg>0){if($E+5<$Yg)echo$Yc;echo($Ed&&$xe!==false?pagination($Yg,$E):" <a href='".h(remove_from_uri("page")."&page=last")."' title='~$Yg'>".lang(296)."</a>");}echo"</ul></div>";}else{echo"<div id='fieldset-pagination'><ul class='pagination'>",pagination(0,$E);if($E>1)echo$Yc;if($E)echo
pagination($E,$E);if($Yg>$E){echo
pagination($E+1,$E);if($Yg>$E+1)echo$Yc;}echo"</ul></div>";}echo"</fieldset>\n";}echo"<fieldset>","<legend>".lang(297)."</legend><div class='fieldset-content'>";$Rc=($Ed?"":"~ ").$xe;echo
checkbox("all",1,0,($xe!==false?($Ed?"":"~ ").lang(196,$xe):""),"countRows.call(this, '$Rc');")."\n","</div></fieldset>\n";if(Admin::get()->isDataEditAllowed()){echo"<fieldset",($_GET["modify"]?'':' class="jsonly"'),">","<legend>",lang(291),"</legend>";$ok=($_GET["modify"]?"":" data-inline-edit='1'".($pd?"":" disabled"));echo"<div class='fieldset-content'",($_GET["modify"]?"":" title='".lang(286)."'"),">","<input type='submit' class='button' id='modify-save' value='",lang(116),"'",$ok,">","</div>","</fieldset>\n","<fieldset>","<legend>",lang(166)," <span id='selected'></span></legend>","<div class='fieldset-content'>","<input type='submit' class='button' name='edit' value='",lang(39),"'> ","<input type='submit' class='button' name='clone' value='",lang(282),"'> ","<input type='submit' class='button' name='delete' value='",lang(120),"'>",confirm(),"</div>","</fieldset>\n";}$qc=Admin::get()->getDataDumpFormats();foreach((array)$_GET["columns"]as$b){if($b["fun"]){unset($qc['sql']);break;}}if($qc){print_fieldset_start("export",lang(75)." <span id='selected2'></span>","export");echo
html_select("format",$qc,$O->getParameter("exportFormat"));$Ii=Admin::get()->getDumpOutputs();echo($Ii?" ".html_select("output",$Ii,$O->getParameter("exportOutput")):"")," <input type='submit' class='button' name='export' value='".lang(75)."'>\n";print_fieldset_end("export");}echo"</div></div>\n",script("initTableFooter()");}echo"</div>\n";if(Admin::get()->isDataEditAllowed()){echo"<p>","<a href='#import'>",icon("import"),lang(74),"</a>",script("qsl('a').onclick = partial(toggle, 'import');",""),"</p>","<p id='import'",($_POST["import"]?"":" class='hidden'"),">";if(ini_bool("file_uploads"))echo"<input type='file' name='csv_file'> ",html_select("import_format",["csv"=>"CSV,","csv;"=>"CSV;","tsv"=>"TSV"],$O->getParameter("exportFormat"))," <input type='submit' class='button default' name='import' value='".lang(74)."'>",file_upload_form_script("selection_form","csv_file");else
echo
lang(203);echo"</p>";}echo
input_token(),"</form>\n",(!$Ke&&$M?"":script("tableCheck();"));}else
echo"</div>\n";}}if(is_ajax()){ob_end_clean();exit;}}elseif(isset($_GET["variables"])){$ol=isset($_GET["status"]);$T=$ol?lang(159):lang(158);page_header($T,[$T]);$gn=($ol?Admin::get()->getStatusVariables():Admin::get()->getServerVariables());if(!$gn)echo"<p class='message'>",lang(90),"</p>\n";else{echo"<div class='scrollable'><table>\n";foreach($gn
as$K){echo"<tr>";$u=array_shift($K);echo"<th><code class='jush-".DIALECT.($ol?"status":"set")."'>".h($u)."</code></th>";foreach($K
as$X)echo"<td>",nl2br(h($X)),"</td>";echo"</tr>\n";}echo"</table></div>\n";}}elseif(isset($_GET["script"])){header("Content-Type: text/javascript; charset=utf-8");if($_GET["script"]=="db"){$_l=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$f=[];$vc=null;foreach(table_status()as$A=>$R){$f["Comment-$A"]=h($R["Comment"]);if(!is_view($R)||preg_match('~materialized~i',$R["Engine"])){$f["Engine-$A"]=h($R["Engine"]);$Cb=isset($R["Collation"])?$R["Collation"]:"";if($Cb==""){if($vc===null)$vc=(string)db_collation(DB,collations());$Cb=$vc;}$f["Collation-$A"]=h($Cb);foreach($_l+["Auto_increment"=>0,"Rows"=>0]as$u=>$X){if($R[$u]!=""){$X=format_number($R[$u]);if($X>=0)$f["$u-$A"]=($u=="Rows"?format_rows($R):$X);if(isset($_l[$u]))$_l[$u]+=($R["Engine"]!="InnoDB"||$u!="Data_free"?$R[$u]:0);}elseif(array_key_exists($u,$R))$f["$u-$A"]="?";}}}if(function_exists('AdminNeo\db_status'))$_l=db_status();foreach($_l
as$u=>$X)$f["sum-$u"]=format_number($X);echo
json_encode($f,JSON_UNESCAPED_UNICODE);}elseif($_GET["script"]=="kill")Connection::get()->query("KILL ".number($_POST["kill"]));else{$f=[];foreach(count_tables(Admin::get()->getDatabases(false))as$h=>$X){$f["tables-$h"]=$X;$f["size-$h"]=db_size($h);}echo
json_encode($f,JSON_UNESCAPED_UNICODE);}exit;}else{$Vl=array_merge((array)$_POST["tables"],(array)$_POST["views"]);if($Vl&&!$_POST["search"]){$I=true;$hh="";if(DIALECT=="sql"&&$_POST["tables"]&&count($_POST["tables"])>1&&($_POST["drop"]||$_POST["truncate"]||$_POST["copy"]))queries("SET foreign_key_checks = 0");if($_POST["truncate"]){if($_POST["tables"])$I=truncate_tables($_POST["tables"]);$hh=lang(298);}elseif($_POST["move"]){$I=move_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$hh=lang(299);}elseif($_POST["copy"]){$I=copy_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$hh=lang(300);}elseif($_POST["drop"]){if($_POST["views"])$I=drop_views($_POST["views"]);if($I&&$_POST["tables"])$I=drop_tables($_POST["tables"]);$hh=lang(301);}elseif(DIALECT=="sqlite"&&$_POST["check"]){foreach((array)$_POST["tables"]as$Q){foreach(get_rows("PRAGMA integrity_check(".q($Q).")")as$K)$hh
.="<b>".h($Q)."</b>: ".h($K["integrity_check"])."<br>";}}elseif(DIALECT!="sql"){$I=(DIALECT=="sqlite"?queries("VACUUM"):apply_queries("VACUUM".($_POST["optimize"]?" ANALYZE":""),(array)$_POST["tables"]));$hh=lang(302);}elseif(!$_POST["tables"])$hh=lang(79);elseif($I=queries(($_POST["optimize"]?"OPTIMIZE":($_POST["check"]?"CHECK":($_POST["repair"]?"REPAIR":"ANALYZE")))." TABLE ".implode(", ",array_map('AdminNeo\idf_escape',$_POST["tables"])))){while($K=$I->fetchAssoc())$hh
.="<b>".h($K["Table"])."</b>: ".h($K["Msg_text"])."<br>";}queries_redirect($_SERVER["REQUEST_URI"],$hh,(bool)$I);}if($_GET["ns"]=="")page_header(lang(31).": ".h(DB),true);else
page_header(lang(191).": ".h($_GET["ns"]),true);Admin::get()->printDatabaseMenu();if($_GET["ns"]===""){echo"<h2 id='schemas'>".lang(303)."</h2>\n";$vk=Admin::get()->getSchemas();if(!$vk)echo"<p class='message'>".lang(304)."\n";else{echo"<div class='scrollable'>\n","<table class='nowrap'>\n",'<thead><tr class="wrap"><th>',lang(191),"</th></tr></thead>";foreach($vk
as$A)echo"<tr><th><a href='",h(ME),"ns=".urlencode($A),"' title='",lang(305),"'>".h($A)."</a></th></tr>";echo'</table></div>';}echo'<p class="links"><a href="'.h(ME).'scheme=">'.icon("database-add").lang(77)."</a>\n";}else{echo"<h2 id='tables-views'>".lang(306)."</h2>\n";$Ql=['sql'=>'show-table-status.html','mariadb'=>'reference/sql-statements/administrative-sql-statements/show/show-table-status'];$vc=db_collation(DB,collations());$c=["Engine"=>["label"=>lang(170),"doc"=>doc_link(['sql'=>'storage-engines.html','mariadb'=>'server-usage/storage-engines']),],];if($vc!="")$c["Collation"]=["label"=>lang(47),"doc"=>doc_link(['sql'=>'charset-charsets.html','mariadb'=>'reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations']),];$c+=["Data_length"=>["label"=>lang(307),"doc"=>doc_link($Ql+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT','oracle'=>'REFRN20286']),"link"=>"create","title"=>lang(36),],"Index_length"=>["label"=>lang(308),"doc"=>doc_link($Ql+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT']),"link"=>"indexes","title"=>lang(176),],"Data_free"=>["label"=>lang(309),"doc"=>doc_link($Ql),"link"=>"edit","title"=>lang(8),],"Auto_increment"=>["label"=>lang(49),"doc"=>doc_link(['sql'=>'example-auto-increment.html','mariadb'=>'reference/data-types/auto_increment']),"link"=>"auto_increment=1&create","title"=>lang(36),],"Rows"=>["label"=>lang(310),"doc"=>doc_link($Ql+['pgsql'=>'catalog-pg-class.html#CATALOG-PG-CLASS','oracle'=>'REFRN20286']),"link"=>"select","title"=>lang(34),],];if(support("comment"))$c["Comment"]=["label"=>lang(48),"doc"=>doc_link($Ql+['pgsql'=>'functions-info.html#FUNCTIONS-INFO-COMMENT-TABLE']),];$D=(is_string($_GET["order"])?$_GET["order"]:"");$Ic=null;if(preg_match('~^(.+)-(asc|desc)$~',$D,$z)){$D=$z[1];$Ic=($z[2]=="desc");}if($D!="__table"&&!isset($c[$D]))$D="";if($Ic===null)$Ic=isset($c[$D]["link"]);$An=($D!=""&&$D!="__table")||support("fast_status");$Tl=($An?table_status():tables_list());if(!$Tl)echo"<p class='message'>".lang(79)."\n";else{echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n";if(support("table")){echo"<div class='field-sets'>\n","<fieldset><legend>".lang(311)." <span id='selected2'></span></legend><div class='fieldset-content'>",html_select("op",Admin::get()->getOperators(),isset($_POST["op"])?$_POST["op"]:Driver::get()->getLikeOperator()),"<input type='search' class='input' name='query' value='".h($_POST["query"])."'>",script("qsl('input').onkeydown = event => bodyKeydown(event, 'search');","")," <input type='submit' class='button' name='search' value='".lang(60)."'>\n","</div></fieldset>\n","</div>\n";if($_POST["search"]&&$_POST["query"]!=""){$_GET["where"][0]["op"]=$_POST["op"];search_tables();}}echo"<div class='scrollable'>\n","<table class='nowrap checkable'>\n",'<thead><tr class="wrap">','<th class="actions"><input id="check-all" type="checkbox" class="input jsonly" title="'.lang(190).'">'.script("gid('check-all').onclick = partial(formCheck, /^(tables|views)\[/);","");$Ch=($D==""||$D=="__table");$Ll=($Ch&&!$Ic?ME."order=__table-desc":substr(ME,0,-1));$Eh=($Ch&&DIALECT!="sqlite");echo'<th'.($Eh?" aria-sort='".($Ic?"descending":"ascending")."'":'').'><a href="'.h($Ll).'">'.lang(9).'</a>';foreach($c
as$u=>$b){$Lc=($u===$D?!$Ic:isset($b["link"]));echo'<th'.($u===$D?" aria-sort='".($Ic?"descending":"ascending")."'":'').'><a href="'.h(ME)."order=$u-".($Lc?"desc":"asc").'">'.$b["label"].'</a>'.$b["doc"];}echo"</thead>\n","<tbody>\n";if($D=="__table"){if($Ic)$Tl=array_reverse($Tl,true);}elseif($D){uasort($Tl,function($sa,$Sa)use($D,$Ic){$Bn=isset($sa[$D])?$sa[$D]:null;$Cn=isset($Sa[$D])?$Sa[$D]:null;$I=($Bn<$Cn?-1:($Bn>$Cn?1:0));return($Ic?-$I:$I);});}$_l=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$S=0;foreach($Tl
as$A=>$ol){$kn=($An?is_view($ol):$ol!==null&&!preg_match('~table|sequence~i',$ol));$vd=($An?(isset($ol["Engine"])?$ol["Engine"]:""):$ol);$r=h("Table-".$A);echo'<tr><th class="actions">'.checkbox(($kn?"views[]":"tables[]"),$A,in_array("$A",$Vl,true),"","","",$r);if(!Admin::get()->getSettings()->isSelectionPreferred()&&(support("table")||support("indexes")))$ua="table";else$ua="select";echo"<th><a href='",h(ME),"$ua=",urlencode($A),"' id='$r'>",h($A),"</a></th>";if($kn&&!preg_match('~materialized~i',$vd)){$T=lang(169);$Ib=count($c)-(support("comment")?2:1);echo'<td colspan="'.$Ib.'">'.(support("view")?"<a href='".h(ME)."view=".urlencode($A)."' title='".lang(37)."'>$T</a>":$T),"<td align='right'><a href='".h(ME)."select=".urlencode($A)."' title='".lang(34)."'>?</a>";}else{foreach($c
as$u=>$b){if($u=="Comment")continue;$r=" id='$u-".h($A)."'";$x=isset($b["link"])?$b["link"]:"";if(!$x){$X="";if($An){$X=isset($ol[$u])?$ol[$u]:"";if($u=="Collation"&&$X=="")$X=$vc;}echo"<td$r>".h($X);continue;}$X="?";if($An){$B=isset($ol[$u])?$ol[$u]:"";if(is_numeric($B)&&$B>=0){$X=($u=="Rows"?format_rows($ol):format_number($B));if(isset($_l[$u])&&($vd!="InnoDB"||$u!="Data_free"))$_l[$u]+=$B;}}echo"<td align='right'>".(support("table")||$u=="Rows"||(support("indexes")&&$u!="Data_length")?"<a href='".h(ME."$x=").urlencode($A)."'$r title='".$b["title"]."'>".h($X)."</a>":"<span$r>".h($X)."</span>");}$S++;}echo(support("comment")?"<td id='Comment-".h($A)."'>".($An?h(isset($ol["Comment"])?$ol["Comment"]:""):""):""),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});"),"<tfoot><tr>","<td><th>".lang(283,count($Tl)),"<td>".h(DIALECT=="sql"?Connection::get()->getValue("SELECT @@default_storage_engine"):""),($vc!=""?"<td>".h($vc):"");if($An&&function_exists('AdminNeo\db_status'))$_l=db_status();foreach($_l
as$u=>$zl)echo"<td align='right' id='sum-$u'>".($An?format_number($zl):"");echo"<td></td><td></td>";if(support("comment"))echo"<td></td>";echo"</tr></tfoot>\n","</table>\n","</div>\n",($An?"":script("ajaxSetHtml('".js_escape(ME)."script=db');"));if(Admin::get()->isDataEditAllowed()){echo"<div class='table-footer'><div class='field-sets'>\n";$dn="<input type='submit' class='button' value='".lang(312)."'> ".help_script("VACUUM");$qi="<input type='submit' class='button' name='optimize' value='".lang(313)."'> ".help_script(DIALECT=="sql"?"OPTIMIZE TABLE":"VACUUM ANALYZE");echo"<fieldset><legend>".lang(166)." <span id='selected'></span></legend><div class='fieldset-content'>".(DIALECT=="sqlite"?$dn."<input type='submit' class='button' name='check' value='".lang(314)."'> ".help_script("PRAGMA integrity_check"):(DIALECT=="pgsql"?$dn.$qi:(DIALECT=="sql"?"<input type='submit' class='button' value='".lang(315)."'> ".help_script("ANALYZE TABLE").$qi."<input type='submit' class='button' name='check' value='".lang(314)."'> ".help_script("CHECK TABLE")."<input type='submit' class='button' name='repair' value='".lang(316)."'> ".help_script("REPAIR TABLE"):"")))."<input type='submit' class='button' name='truncate' value='".lang(317)."'> ".help_script(DIALECT=="sqlite"?"DELETE":("TRUNCATE".(DIALECT=="pgsql"?"":" TABLE"))).confirm()."<input type='submit' class='button' name='drop' value='".lang(167)."'>".help_script("DROP TABLE").confirm()."\n";$g=(support("scheme")?Admin::get()->getSchemas():Admin::get()->getDatabases());echo"</div></fieldset>\n";if(count($g)!=1&&DIALECT!="sqlite"){echo"<fieldset><legend>".lang(318)." <span id='selected3'></span></legend><div>";$h=(isset($_POST["target"])?$_POST["target"]:(support("scheme")?$_GET["ns"]:DB));echo($g?html_select("target",$g,$h,"","label-move"):'<input class="input" name="target" value="'.h($h).'" autocapitalize="off">')," <input type='submit' class='button' name='move' value='".lang(319)."'>",(support("copy")?" <input type='submit' class='button' name='copy' value='".lang(320)."'> ".checkbox("overwrite",1,$_POST["overwrite"],lang(321)):""),"</div></fieldset>\n";}echo
input_hidden("all"),script("qsl('input').onclick = partial(countTables, $S);"),input_token(),"</div></div>\n",script("initTableFooter()");}echo"</div>\n","</form>\n",script("tableCheck();");}echo'<p class="links"><a href="',h(ME),'create=">',icon("table-add"),lang(78),"</a>\n";if(support("view"))echo'<a href="',h(ME),'view=">',icon("view-add"),lang(246),"</a>\n";if(support("routine")){echo"<h2 id='routines'>".lang(186)."</h2>\n";$kk=routines();if($kk){$Ob=$kk[0]["ROUTINE_COMMENT"]!==null;echo"<table>\n",'<thead><tr>','<th>',lang(226),'</th><td>',lang(45),'</td><td>',lang(263),"</td>";if($Ob)echo"<td>",lang(48),"</td>";echo"<td></td>","</tr></thead>\n";foreach($kk
as$K){$A=($K["SPECIFIC_NAME"]==$K["ROUTINE_NAME"]?"":"&name=".urlencode($K["ROUTINE_NAME"]));echo'<tr>','<th><a href="',h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'callf=':'call=').urlencode($K["SPECIFIC_NAME"]).$A),'">',h($K["ROUTINE_NAME"]),'</a></th>','<td>',h($K["ROUTINE_TYPE"]),'</td>','<td>',h($K["DTD_IDENTIFIER"]),'</td>';if($Ob)echo'<td>',truncate_utf8(preg_replace('~\s{2,}~'," ",trim($K["ROUTINE_COMMENT"])),50),'</td>';echo'<td><a href="'.h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'function=':'procedure=').urlencode($K["SPECIFIC_NAME"]).$A).'">'.lang(179)."</a></td>";}echo"</table>\n";}echo'<p class="links">';if(support("procedure"))echo'<a href="',h(ME),'procedure=">',icon("function-add"),lang(262),"</a>";echo'<a href="',h(ME),'function=">',icon("function-add"),lang(261),"</a>\n","</p>\n";}if(support("event")){echo"<h2 id='events'>".lang(187)."</h2>\n";$L=get_rows("SHOW EVENTS");if($L){echo"<table>\n","<thead><tr><th>".lang(226)."<td>".lang(322)."<td>".lang(252)."<td>".lang(253)."<td></thead>\n";foreach($L
as$K)echo"<tr>","<th>".h($K["Name"]),"<td>".($K["Execute at"]?lang(323)."<td>".h($K["Execute at"]):lang(254)." ".h($K["Interval value"])." ".h($K["Interval field"])."<td>".h($K["Starts"])),"<td>".h($K["Ends"]),'<td><a href="'.h(ME).'event='.urlencode($K["Name"]).'">'.lang(179).'</a>';echo"</table>\n";$Dd=Connection::get()->getValue("SELECT @@event_scheduler");if($Dd&&$Dd!="ON")echo"<p class='error'><code class='jush-sqlset'>event_scheduler</code>: ".h($Dd)."\n";}echo'<p class="links"><a href="',h(ME),'event=">',icon("event-add"),lang(251),"</a></p>\n";}}}page_footer();