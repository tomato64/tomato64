<?php
/**
 * AdminNeo - Powerful database manager in a single PHP file
 * v5.7.1
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
inject($ya,Config$Sb,Settings$O,Locale$qg){$this->admin=$ya;$this->config=$Sb;$this->settings=$O;$this->locale=$qg;}}abstract
class
Origin
extends
Plugin{private$errors=[];private
static$instance=null;static
function
create(array$Sb=[],array$Bi=[]){if(self::$instance)die("Admin instance already exists.\n");$ya=new
static();if(!$Sb&&file_exists("adminneo-config.php")){$Sb=include_once("adminneo-config.php");if(!is_array($Sb)){$Sb=[];$ig="href=https://github.com/adminneo-org/adminneo#configuration ".target_blank();$ya->addError(lang(0,"<b>adminneo-config.php</b>")." <a $ig>".lang(1)."</a>");}}$Sb=new
Config($Sb);$O=new
Settings($Sb);if(!$Bi&&file_exists("adminneo-plugins.php")){$Bi=include_once("adminneo-plugins.php");if(!is_array($Bi)){$Bi=[];$ig="href=https://github.com/adminneo-org/adminneo#plugins ".target_blank();$ya->addError(lang(0,"<b>adminneo-plugins.php</b>")." <a $ig>".lang(1)."</a>");}}self::$instance=$Bi?new
Pluginer($ya,$Bi):$ya;$ya->inject(self::$instance,$Sb,$O,Locale::get());foreach($Bi
as$Ai)$Ai->inject(self::$instance,$Sb,$O,Locale::get());return
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
addError($i){$this->errors[]=$i;}function
getErrors(){return$this->errors;}abstract
function
getServiceTitle();function
getCredentials(){$N=$this->config->getServer(SERVER);return[$N?$N->getServer():SERVER,$_GET["username"],get_password()];}function
verifyDefaultPassword($F){$Ge=$this->config->getDefaultPasswordHash();if($Ge===null||$Ge==="")return
lang(2);elseif(!password_verify($F,$Ge))return
lang(3);return
true;}function
authenticate($V,$F){if($F==""){$Ge=$this->config->getDefaultPasswordHash();if($Ge===null)return
lang(4,target_blank());else
return$Ge==="";}return
true;}function
getPrivateKey($cc=false){return
get_private_key($cc);}function
getBruteForceKey(){return$_SERVER["REMOTE_ADDR"];}function
getServerName($N,$sj=true,$Hd=null){if($N==""){if(!$sj)return"";$N=Connection::exists()?Connection::get()->getDefaultServerName():"";if($N=="")return$Hd!==null?$Hd:lang(5);$ck=null;}else$ck=$this->config->getServer($N);return$ck?$ck->getName():preg_replace('~^https?://~',"",$N);}abstract
function
getDatabase();function
getDatabases($ae=true){$f=$this->filterListWithWildcards(get_databases($ae),$this->config->getHiddenDatabases(),false,Driver::get()->getSystemDatabases());if(DB!=""&&!in_array(DB,$f))array_unshift($f,DB);return$f;}function
getSchemas($lh=false){$Je=$this->config->getHiddenSchemas();if($lh&&!in_array("__system",$Je))$Je[]="__system";$Mj=$this->filterListWithWildcards(schemas(),$Je,false,Driver::get()->getSystemSchemas());if(isset($_GET["ns"])&&$_GET["ns"]!=""&&!in_array($_GET["ns"],$Mj))array_unshift($Mj,$_GET["ns"]);return$Mj;}function
getCollations(array$Ff=[]){$wm=$this->config->getVisibleCollations();$Ud=$wm?array_merge($wm,$Ff):[];return$this->filterListWithWildcards(collations(),$Ud,true);}private
function
filterListWithWildcards(array$nm,array$Ud,$Hf,array$Sk=[]){if(!$nm||!$Ud)return$nm;$r=array_search("__system",$Ud);if($r!==false){unset($Ud[$r]);$Ud=array_merge($Ud,$Sk);}array_walk($Ud,function(&$Y){$Y=str_replace('\\*',".*",preg_quote($Y,"~"));});$vi='~^('.implode("|",$Ud).')$~';return$this->filterListWithPattern($nm,$vi,$Hf);}private
function
filterListWithPattern(array$nm,$vi,$Hf){$I=[];foreach($nm
as$t=>$Y){if(is_array($Y)){if($Ik=$this->filterListWithPattern($Y,$vi,$Hf))$I[$t]=$Ik;}elseif(($Hf&&preg_match($vi,$Y))||(!$Hf&&!preg_match($vi,$Y)))$I[$t]=$Y;}return$I;}abstract
function
getQueryTimeout();function
sendHeaders(){}function
updateCspHeader(array&$gc){}function
printFavicons(){$Db=validate_color_variant($this->config->getColorVariant());echo"<link rel='icon' type='image/x-icon' href='",link_files("favicon-$Db.ico",[]),"' sizes='32x32'>\n","<link rel='icon' type='image/svg+xml' href='",link_files("favicon-$Db.svg",[]),"'>\n","<link rel='apple-touch-icon' href='",link_files("apple-touch-icon-$Db.png",[]),"'>\n";}abstract
function
printToHead();function
getCssUrls(){$cm=$this->config->getCssUrls();foreach(["adminneo.css","adminneo-light.css","adminneo-dark.css"]as$m){if(file_exists($m))$cm[]="$m?v=".filemtime($m);}return$cm;}function
isLightModeForced(){return$this->isColorSchemeForced(false);}function
isDarkModeForced(){return$this->isColorSchemeForced(true);}private
function
isColorSchemeForced($lc){$Rg=$lc?Settings::$ColorSchemeDark:Settings::$ColorSchemeLight;$Sg=$lc?Settings::$ColorSchemeLight:Settings::$ColorSchemeDark;$Qd=file_exists("adminneo-$Rg.css");$Rd=file_exists("adminneo-$Sg.css");if($Qd&&!$Rd)return
true;return$this->settings->getColorScheme()==$Rg&&!($Qd
xor$Rd);}function
getJsUrls(){$cm=$this->config->getJsUrls();$m="adminneo.js";if(file_exists($m))$cm[]="$m?v=".filemtime($m);return$cm;}abstract
function
printLoginForm();function
getLoginFormRow($Ld,$Pf,$j){if($Pf)return"<tr><th>$Pf</th><td>$j</td></tr>\n";else
return"$j\n";}function
printLogout(){echo"<div class='logout'>","<form action='' method='post'>\n",h($_GET["username"]),"<input type='submit' class='button' name='logout' value='",lang(6),"' id='logout'>",input_token(),"</form>","</div>\n";}function
getTableName(array$Wk){return
h($Wk["Name"]);}abstract
function
getFieldName(array$j,$D=0);function
formatComment($Kb){return
h($Kb);}abstract
function
printTableMenu(array$Wk,$mf);function
getForeignKeys($Q){return
foreign_keys($Q);}function
getBackwardKeys($Q,$Uk){if(!$this->settings->isRelationLinks())return[];$L=backward_keys($Q);$Jf=[];foreach($L
as$K){$q=$K["table_schema"].".".$K["table_name"];$Jf[$q]["schema"]=$K["table_schema"];$Jf[$q]["table"]=$K["table_name"];$Jf[$q]["constraints"][$K["constraint_name"]][$K["column_name"]]=$K["referenced_column_name"];}foreach($Jf
as$q=>$t){$A=$this->admin->getTableName(table_status1($t["table"],true));if($A!=""){$Pj=preg_quote($Uk);$Zj="(:|\\s*-)?\\s+";$Jf[$q]["name"]=(preg_match("(^$Pj$Zj(.+)|^(.+?)$Zj$Pj\$)iu",$A,$y)?$y[2].$y[3]:$A);}else
unset($Jf[$q]);}return$Jf;}function
printBackwardKeys(array$Ua,array$K){foreach($Ua
as$t){foreach($t["constraints"]as$Vb){$Dg=preg_replace('~&ns=[^&]+&~',"&ns=".urldecode($t["schema"])."&",ME);$w=$Dg.'select='.urlencode($t["table"]);$p=0;foreach($Vb
as$b=>$X){if(!isset($K[$X]))continue
2;$w
.=where_link($p++,$b,$K[$X]);}$A=preg_replace('(^'.preg_quote($_GET["select"]).(substr($_GET["select"],-1)=="s"?"?":"").'_)',"_",$t["name"]);$T=implode(", ",array_keys($Vb));echo"<a href='".h($w)."' title='".h($T)."'>".h($A)."</a>";$w=$Dg.'edit='.urlencode($t["table"]);foreach($Vb
as$b=>$X)$w
.="&preset".urlencode("[".bracket_escape($b)."]")."=".urlencode($K[$X]);echo"<a href='".h($w)."' title='".lang(7)."'>",icon_solo("add"),"</a> ";}}}abstract
function
formatSelectQuery($H,$Ak,$Gd=false);abstract
function
formatMessageQuery($H,$vl,$Gd=false);abstract
function
formatSqlCommandQuery($H);function
printAfterSqlCommand(){}abstract
function
getTableDescriptionFieldName($Q);abstract
function
fillForeignDescriptions(array$L,array$de);function
getFieldValueLink($X,$j){if(is_mail($X))return"mailto:$X";if(is_web_url($X))return$X;return
null;}abstract
function
formatSelectionValue($X,$w,$j,$Zh);abstract
function
formatFieldValue($Y,array$j);abstract
function
printTableStructure(array$k);abstract
function
printTablePartitions(array$li);abstract
function
printRelatedTables(array$S);abstract
function
printTableIndexes(array$s,array$Wk);abstract
function
printSelectionColumns(array$M,array$c);abstract
function
printSelectionSearch(array$Z,array$c,array$s);abstract
function
printSelectionOrder(array$D,array$c,array$s);abstract
function
printSelectionLimit($v);abstract
function
printSelectionLength($ql);abstract
function
printSelectionAction(array$s);function
isDataEditAllowed(){return!information_schema(DB);}abstract
function
processSelectionColumns(array$c,array$s);abstract
function
processSelectionSearch(array$k,array$s);abstract
function
processSelectionOrder(array$k,array$s);function
processSelectionLimit(){if(!isset($_GET["limit"]))return$this->settings->getRecordsPerPage();return$_GET["limit"]!=""?(int)$_GET["limit"]:0;}abstract
function
processSelectionLength();abstract
function
getFieldFunctions(array$j);abstract
function
getFieldInput($Q,array$j,$Ma,$Y,$o);function
getFieldInputHint($Q,array$j,$Y){return
support("comment")?$this->admin->formatComment($j["comment"]):"";}abstract
function
processFieldInput(array$j,$Y,$o="");function
detectJson($Md,&$Y,$Mi=null){if(is_array($Y)){$Yd=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($this->config->isJsonValuesAutoFormat()?JSON_PRETTY_PRINT:0);$Y=json_encode($Y,$Yd);return
true;}$Yd=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($Mi?JSON_PRETTY_PRINT:0);if(preg_match('~^jsonb?$~',$Md)){if($Y!=null&&$Mi!==null&&$this->config->isJsonValuesAutoFormat())$Y=json_encode(json_decode($Y),$Yd);return
true;}if(!$this->config->isJsonValuesDetection())return
false;if(is_string($Y)&&$Y!=""&&preg_match('~varchar|text|character varying|String|keyword~',$Md)&&($Y[0]=="{"||$Y[0]=="[")&&($Df=json_decode($Y))){if($Mi!==null&&$this->config->isJsonValuesAutoFormat())$Y=json_encode($Df,$Yd);return
true;}return
false;}function
getServerVariables(){return
show_variables();}function
getStatusVariables(){return
show_status();}abstract
function
getDumpOutputs();abstract
function
getDumpFormats();abstract
function
sendDumpHeaders($Ue,$Vg=false);function
dumpDatabase($oc){}abstract
function
dumpTable($Q,$Hk,$tm=0);abstract
function
dumpData($Q,$Hk,$H);abstract
function
getImportFilePath();abstract
function
printDatabaseMenu();function
printNavigation($Pg){$Wf=isset($_COOKIE["neo_version"])?$_COOKIE["neo_version"]:null;echo"<div class='header'>\n",$this->admin->getServiceTitle()."\n";if($Pg!="auth"){echo"<span class='version'>",h(preg_replace('~\\.0(-|$)~','$1',VERSION));if($this->config->isVersionVerificationEnabled()&&$Wf&&version_compare(VERSION,$Wf)<0)echo"<a id='version' class='version-badge' href='https://www.adminneo.org/download' ".target_blank()." title='".h($Wf)."'>",icon_solo("asterisk"),"</a>";echo"</span>\n";if($this->config->isVersionVerificationEnabled()&&!$Wf)echo
script("verifyVersion('".js_escape(ME)."', '".get_token()."');");}echo"</div>\n";}abstract
function
printDatabaseSwitcher($Pg);function
printTablesFilter(){echo"<div class='tables-filter jsonly'>"."<input id='tables-filter' type='search' class='input' autocomplete='off' placeholder='".lang(8)."'>".script("initTablesFilter(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");")."</div>\n";}abstract
function
printTableList(array$S);function
getSettingsRows($ze){$O=[];if($ze==1){$C=get_language_options();if($C)$O["lang"]="<tr><th id='label-language'>".lang(9)."</th>"."<td>".html_select("lang",get_language_options(),Locale::get()->getLanguage(),"","label-language")."</td></tr>\n";$C=[""=>lang(10),Settings::$ColorSchemeLight=>lang(11),Settings::$ColorSchemeDark=>lang(12)];$O["colorScheme"]="<tr><th>".lang(13)."</th>"."<td>".html_radios("colorScheme",$C,($ra=$this->settings->getParameter("colorScheme"))!==null?$ra:"")."</td></tr>\n";}elseif($ze==2){$C=[""=>lang(14),true=>lang(15),false=>lang(16),];$h=$C[$this->config->isRelationLinks()];$C[""].=" ($h)";$O["relationLinks"]="<tr><th>".lang(17)."</th>"."<td>".html_radios("relationLinks",$C,($ra=$this->settings->getParameter("relationLinks"))!==null?$ra:"")."<span class='input-hint'>".lang(18)."</span>"."</td></tr>\n";$h=$this->config->getRecordsPerPage();$C=[""=>lang(14)." ($h)","20","30","50","70","100",];$O["recordsPerPage"]="<tr><th id='label-records'>".lang(19)."</th>"."<td>".html_select("recordsPerPage",$C,($ra=$this->settings->getParameter("recordsPerPage"))!==null?$ra:"","","label-records")."<span class='input-hint'>".lang(20)."</span>"."</td></tr>\n";$h=($ra=$this->config->getEnumAsSelectThreshold())!==null?$ra:lang(21);$C=[""=>lang(14)." ($h)",-1=>lang(21),0=>lang(22),3=>lang(23,3),5=>lang(23,5),10=>lang(23,10),20=>lang(23,20),];$O["enumAsSelectThreshold"]="<tr><th id='label-enum'>".lang(24)."</th>"."<td>".html_select("enumAsSelectThreshold",$C,($ra=$this->settings->getParameter("enumAsSelectThreshold"))!==null?$ra:"","","label-enum",true)."<span class='input-hint'>".lang(25)."</span>"."</td></tr>\n";}return$O;}abstract
function
getForeignColumnInfo(array$de,$b);}class
Pluginer{private
static$InternalMethods=["inject"=>true,"getConfig"=>true,];private
static$AppendMethods=["getErrors"=>true,"getFieldFunctions"=>true,"getDumpOutputs"=>true,"getDumpFormats"=>true,"getSettingsRows"=>true,];private$plugins;private$hooks=[];function
__construct(Origin$ya,array$Bi){$this->plugins=$Bi;foreach(get_class_methods('\AdminNeo\Origin')as$Ng){$this->hooks[$Ng]=[];if(!(isset(self::$InternalMethods[$Ng])?self::$InternalMethods[$Ng]:false)){foreach($Bi
as$Ai){if(method_exists($Ai,$Ng))$this->hooks[$Ng][]=$Ai;}}if(isset(self::$AppendMethods[$Ng])?self::$AppendMethods[$Ng]:false)array_unshift($this->hooks[$Ng],$ya);else$this->hooks[$Ng][]=$ya;}}function
getPlugins(){return$this->plugins;}function
__call($A,array$gi){$Ha=isset(self::$AppendMethods[$A])?self::$AppendMethods[$A]:false;$I=$Ha?[]:null;assert(isset($this->hooks[$A]),"Calling unknown plugin method: $A");foreach($this->hooks[$A]as$Ai){$Y=call_user_func_array([$Ai,$A],$gi);if($Y!==null){if($Ha)$I+=$Y;else
return$Y;}}return$I;}function
updateCspHeader(array&$gc){$this->__call(__FUNCTION__,[&$gc]);}function
detectJson($Md,&$Y,$Mi=null){return$this->__call(__FUNCTION__,[$Md,&$Y,$Mi]);}}class
Admin
extends
Origin{function
getOperators(){return
Driver::get()->getOperators();}function
getServiceTitle(){return"<a href='".h(HOME_URL)."'><svg role='img' class='logo' width='133' height='28'><desc>AdminNeo</desc><use href='".link_files("logo.svg",[])."#logo'/></svg></a>";}function
getDatabase(){return
DB;}function
getQueryTimeout(){return
2;}function
printToHead(){echo"<link rel='stylesheet' href='",link_files("jush.css",[]),"'>";if(!$this->admin->isLightModeForced())echo"<link rel='stylesheet' ".(!$this->admin->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("jush-dark.css",[]),"'>\n";echo
script_src(link_files("jush.js",[]),true);}function
printLoginForm(){$Sc=Drivers::getList();$dk=$this->config->getServerPairs($Sc);$N=SERVER?:$this->config->getDefaultServer();echo"<table class='box box-light'>\n";if($dk)echo$this->admin->getLoginFormRow('server',lang(5),"<select name='auth[server]'>".optionlist($dk,$N,true)."</select>");else{$Qc=DRIVER?:$this->config->getDefaultDriver($Sc);if(count($Sc)>1)echo$this->admin->getLoginFormRow('driver',lang(26),html_select("auth[driver]",$Sc,$Qc).script("initLoginDriver(qsl('select'));",""));else
echo$this->admin->getLoginFormRow('driver','',input_hidden("auth[driver]",$Qc));echo$this->admin->getLoginFormRow('server',lang(5),"<input class='input' name='auth[server]' value='".h($N)."' title='".lang(27)."' placeholder='localhost' autocapitalize='off'>");}echo$this->admin->getLoginFormRow('username',lang(28),'<input class="input" name="auth[username]" id="username" value="'.h($_GET["username"]).'" autocomplete="username" autocapitalize="off">'),$this->admin->getLoginFormRow('password',lang(29),'<input type="password" class="input" name="auth[password]" autocomplete="current-password">');if(!$dk){$oc=isset($_GET["db"])?$_GET["db"]:$this->config->getDefaultDatabase();echo$this->admin->getLoginFormRow('db',lang(30),'<input class="input" name="auth[db]" value="'.h($oc).'" autocapitalize="off">');}echo"</table>\n","<p>","<input type='submit' class='button default' value='".lang(31)."'>",checkbox("auth[permanent]",1,$_COOKIE["neo_permanent"],lang(32)),"</p>\n";}function
getFieldName(array$j,$D=0){$U=$j["full_type"].($j["null"]?" NULL":"");$Kb=$j["comment"];$Zj=$U&&$Kb!=""?": ":"";return'<span title="'.h($U.$Zj.$Kb).'">'.h($j["field"]).'</span>';}function
printTableMenu(array$Wk,$mf){echo'<p class="links top-tabs">';$jg=[];$Vj=($this->settings->isSelectionPreferred()&&!$this->settings->isNavigationReversed())||(!$this->settings->isSelectionPreferred()&&$this->settings->isNavigationReversed());if($Vj)$jg["select"]=[lang(33),"data"];if(support("table")||support("indexes"))$jg["table"]=[lang(34),"structure"];if(!$Vj)$jg["select"]=[lang(33),"data"];$Q=$Wk["Name"];$_f=false;if(support("table")){$_f=is_view($Wk);if(!$_f){if($Q!="")$jg["create"]=[lang(35),"edit"];}elseif(support("view"))$jg["view"]=[lang(36),"edit"];}if($mf!==null)$jg["edit"]=[lang(7),"item-add"];$gi=$mf?"&".http_build_query($mf):"";foreach($jg
as$t=>$X)echo" <a href='",h(ME),"$t=",urlencode($Q),($t=="edit"?$gi:""),"'",bold(isset($_GET[$t])),">",icon($X[1]),"$X[0]</a>";echo
doc_link([DIALECT=>Driver::get()->tableHelp($Q,$_f)],icon("help").lang(37)),"\n";}function
formatSelectQuery($H,$Ak,$Gd=false){$Nk=support("sql");$_m=!$Gd?Driver::get()->warnings():null;if($Nk)$H
.=";";$Qk=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J="<pre><code class='jush-$Qk'>".h(str_replace("\n"," ",$H))."</code></pre>\n";$J
.="<p class='links'>";if($Nk)$J
.="<a href='".h(ME)."sql=".urlencode($H)."'>".icon("edit").lang(38)."</a>";if($_m)$J
.="<a href='#warnings' class='toggle'>".lang(39).icon_chevron_down()."</a>";$J
.=" <span class='time'>(".format_time($Ak).")</span>";$J
.="</p>\n";if($_m){$J
.=script("initToggles(qsl('p'));");$J
.="<div id='warnings' class='warnings hidden'>\n$_m\n</div>\n";}return$J;}function
formatMessageQuery($H,$vl,$Gd=false){restart_session();$Le=&get_session("queries");if(!isset($Le[$_GET["db"]]))$Le[$_GET["db"]]=[];if(strlen($H)>1e6)$H=preg_replace('~[\x80-\xFF]+$~','',substr($H,0,1e6))."\n…";$Le[$_GET["db"]][]=[$H,time(),$vl];$Nk=support("sql");$_m=!$Gd?Driver::get()->warnings():null;$yk="sql-".count($Le[$_GET["db"]]);$Am="warnings-".count($Le[$_GET["db"]]);$J=" ";if($_m)$J
.="<a href='#$Am' class='toggle'>".lang(39).icon_chevron_down()."</a>, ";$Yi=support("sql")?lang(40):lang(41);$J
.="<a href='#$yk' class='toggle'>$Yi".icon_chevron_down()."</a>";$J
.=" <span class='time'>".@date("H:i:s")."</span>\n";if($_m)$J
.="<div id='$Am' class='warnings hidden'>\n$_m</div>\n";$J
.="<div id='$yk' class='hidden'>\n";$Qk=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J
.="<pre><code class='jush-$Qk'>".truncate_utf8($H,1000)."</code></pre>\n";$J
.="<p class='links'>";if($Nk)$J
.="<a href='".h(str_replace("db=".urlencode(DB),"db=".urlencode($_GET["db"]),ME).'sql=&history='.(count($Le[$_GET["db"]])-1))."'>".icon("edit").lang(38)."</a>";if($vl)$J
.=" <span class='time'>($vl)</span>";$J
.="</p>\n";$J
.="</div>\n";return$J;}function
formatSqlCommandQuery($H){if(preg_match('~^DELIMITER\s~i',$H))return"";return
truncate_utf8($H,1000);}function
getTableDescriptionFieldName($Q){return"";}function
fillForeignDescriptions(array$L,array$de){return$L;}function
formatSelectionValue($X,$w,$j,$Zh){if($X===null)$pl="<i>NULL</i>";elseif(!$j)$pl=$X;elseif(preg_match("~char|binary|boolean~",$j["type"])&&!preg_match("~var~",$j["type"]))$pl="<code>$X</code>";elseif(is_blob($j)&&!is_utf8($X))$pl="<i>".lang(42,strlen($Zh))."</i>";elseif($this->admin->detectJson($j["full_type"],$Zh))$pl="<code class='jush-json'>$X</code>";else$pl=$X;if($w)$pl="<a href='".h($w)."'".(is_web_url($w)?target_blank():"").">$pl</a>";return$pl;}function
formatFieldValue($Y,array$j){return$Y;}function
printTableStructure(array$k){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th>",lang(43),"</th>","<td>",lang(44),"</td>","<td>",lang(45),"</td>";if(support("comment"))echo"<td>",lang(46),"</td>";echo"</tr></thead>\n";$im=Driver::get()->getUserTypes();foreach($k
as$j){echo"<tr>","<th>",h($j["field"]),"</th>","<td>";$U=h($j["full_type"]);if(in_array($U,$im))echo"<a href='".h(ME.'type='.urlencode($U))."'>$U</a>";else
echo$U;if($j["null"])echo" <i>NULL</i>";if($j["auto_increment"])echo" <i>".lang(47)."</i>";$h=h($j["default"]);if(isset($j["default"]))echo" <span title='".lang(48)."'>[<b>",$j["generated"]?"<code class='jush-".DIALECT."'>$h</code>":$h,"</b>]</span>";echo"</td>","<td>",h($j["collation"]),"</td>";if(support("comment"))echo"<td>",$this->admin->formatComment($j["comment"]),"</td>";echo"\n";}echo"</table>\n","</div>\n";}function
printTablePartitions(array$li){$nk=isset($li["partition_names"]);echo"<p>","<code class='jush-".DIALECT."'>BY {$li["partition_by"]} ({$li["partition"]})</code>";if(!$nk&&isset($li["partitions"]))echo" ".lang(49).": ".h($li["partitions"]);echo"</p>";if($nk){echo"<table>\n","<thead><tr><th>".lang(50)."</th><td>".lang(51)."</td></tr></thead>\n";foreach($li["partition_names"]as$t=>$A){echo"<tr><th>";if(DIALECT=="pgsql")echo"<a href='",h(ME."table=".urlencode($A)),"'>";echo
h($A);if(DIALECT=="pgsql")echo"</a>";echo"</th><td>".h($li["partition_values"][$t])."\n";}echo"</table>\n";}}function
printRelatedTables(array$S){echo"<ul class='links'>\n";foreach($S
as$K){$w=preg_replace('~ns=[^&]*~',"ns=".urlencode($K["ns"]),ME);echo"<li><a href='",h($w."table=".urlencode($K["table"])),"'>",icon("structure");if($K["ns"]!=$_GET["ns"])echo"<b>".h($K["ns"])."</b>.";echo
h($K["table"]),"</a>";}echo"</ul>\n";}function
printTableIndexes(array$s,array$Wk){$tc=first(Driver::get()->getIndexAlgorithms($Wk));$ji=false;foreach($s
as$r){if(isset($r["partial"])?$r["partial"]:false){$ji=true;break;}}echo"<table>\n","<thead><tr>","<th>",lang(44),"</th>","<td>",lang(52)," (",lang(53),")</td>";if($ji)echo"<td>",lang(54),"</td>";echo"</tr></thead>\n";foreach($s
as$A=>$r){ksort($r["columns"]);$Oi=[];foreach($r["columns"]as$t=>$X)$Oi[]="<i>".h($X)."</i>".($r["lengths"][$t]?"(".h($r["lengths"][$t]).")":"").($r["descs"][$t]?" DESC":"");echo"<tr title='",h($A),"'>","<th>",h($r["type"]);if(isset($r['algorithm'])&&$r['algorithm']!=$tc)echo" (",h($r['algorithm']),")";echo"</th>","<td>",implode(", ",$Oi),"</td>";if($ji){echo"<td>";if($r['partial'])echo"<code class='jush-",DIALECT,"'>WHERE ",h($r['partial']),"</code>";echo"</td>";}echo"</tr>\n";}echo"</table>\n";}function
printSelectionColumns(array$M,array$c){print_fieldset_start("select",lang(55),"columns",(bool)$M,true);$M[""]=[];$p=0;foreach($M
as$t=>$X){$X=isset($_GET["columns"][$t])?$_GET["columns"][$t]:[];$b=select_input("name='columns[$p][col]'",$c,isset($X["col"])?$X["col"]:null,$t!==""?"selectFieldChange":"selectAddRow");echo"<div ",($t!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly");if(Driver::get()->getFunctions()||Driver::get()->getGrouping())echo
html_select("columns[$p][fun]",[-1=>""]+array_filter([lang(56)=>Driver::get()->getFunctions(),lang(57)=>Driver::get()->getGrouping()]),isset($X["fun"])?$X["fun"]:null),help_script_command("value && value.replace(/ |\$/, '(') + ')'",true),script("qsl('select').onchange = (event) => { ".($t!==""?"":" qsl('select, input:not(.remove)', event.target.parentNode).onchange();")." };",""),"($b)";else
echo$b;echo" <button class='button light remove jsonly' title='",lang(58),"'>",icon_solo("remove"),"</button>",script("qsl('#fieldset-select .remove').onclick = selectRemoveRow;",""),"</div>\n";$p++;}print_fieldset_end("select",true);}function
printSelectionSearch(array$Z,array$c,array$s){print_fieldset_start("search",lang(59),"search",(bool)$Z);foreach($s
as$p=>$r){if($r["type"]=="FULLTEXT"){echo"<div>(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$r["columns"]))."</i>) AGAINST","<input type='text' class='input' name='fulltext[$p]' value='".h(isset($_GET["fulltext"][$p])?$_GET["fulltext"][$p]:null)."'>",script("qsl('input').oninput = selectFieldChange;","");if(DIALECT=='sql')echo
checkbox("boolean[$p]",1,isset($_GET["boolean"][$p]),"BOOL");echo"</div>\n";}}$mb="this.parentNode.firstChild.onchange();";foreach(array_merge((array)$_GET["where"],[[]])as$p=>$X){if(!$X||("$X[col]$X[val]"!=""&&in_array($X["op"],$this->getOperators())))echo"<div>",select_input(" name='where[$p][col]'",$c,$X["col"],($X?"selectFieldChange":"selectAddRow"),"(".lang(60).")"),html_select("where[$p][op]",$this->getOperators(),$X["op"],$mb),"<input type='text' class='input' name='where[$p][val]' value='".h($X["val"])."'>",script("mixin(qsl('input'), {oninput: function () { $mb }, onkeydown: selectSearchKeydown});","")," <button class='button light remove jsonly' title='".lang(58)."'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-search .remove").onclick = selectRemoveRow;',""),"</div>\n";}print_fieldset_end("search");}function
printSelectionOrder(array$D,array$c,array$s){print_fieldset_start("sort",lang(61),"sort",(bool)$D,true);$_GET["order"][""]="";$p=0;foreach((array)$_GET["order"]as$t=>$X){if($t!=""&&$X=="")continue;echo"<div ",($t!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly"),select_input("name='order[$p]'",$c,$X,$t!==""?"selectFieldChange":"selectAddRow")," ",checkbox("desc[$p]",1,isset($_GET["desc"][$t]),lang(62))," <button class='button light remove jsonly' title='",lang(58),"'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-sort .remove").onclick = selectRemoveRow;',""),"</div>\n";$p++;}print_fieldset_end("sort",true);}function
printSelectionLimit($v){echo"<fieldset><legend>".lang(63)."</legend><div class='fieldset-content'>","<input type='number' name='limit' class='input size' value='$v'>",script("qsl('input').oninput = selectFieldChange;",""),"</div></fieldset>\n";}function
printSelectionLength($ql){if($ql!==null)echo"<fieldset><legend>".lang(64)."</legend><div class='fieldset-content'>","<input type='number' name='text_length' class='input size' value='".h($ql)."'>","</div></fieldset>\n";}function
printSelectionAction(array$s){echo"<fieldset><legend>".lang(65)."</legend><div class='fieldset-content'>","<input type='submit' class='button' value='".lang(55)."'>"," <span id='noindex' title='".lang(66)."'></span>","<script".nonce().">\n";$c=new
stdClass();foreach($s
as$r){$ic=reset($r["columns"]);if($r["type"]!="FULLTEXT"&&$ic)$c->$ic=null;}echo"const indexColumns = ".json_encode($c,JSON_UNESCAPED_UNICODE|JSON_HEX_TAG).";\n","selectFieldChange.call(gid('form')['select']);\n","</script>\n","</div></fieldset>\n";}function
processSelectionColumns(array$c,array$s){$M=[];$xe=[];foreach((array)$_GET["columns"]as$t=>$X){if($X["fun"]=="count"||($X["col"]!=""&&(!$X["fun"]||in_array($X["fun"],Driver::get()->getFunctions())||in_array($X["fun"],Driver::get()->getGrouping())))){$M[$t]=apply_sql_function($X["fun"],($X["col"]!=""?idf_escape($X["col"]):"*"));if(!in_array($X["fun"],Driver::get()->getGrouping()))$xe[]=$M[$t];}}return[$M,$xe];}function
processSelectionSearch(array$k,array$s){$J=[];foreach($s
as$p=>$r){if($r["type"]=="FULLTEXT"&&isset($_GET["fulltext"])&&$_GET["fulltext"][$p]!="")$J[]="MATCH (".implode(", ",array_map('AdminNeo\idf_escape',$r["columns"])).") AGAINST (".q($_GET["fulltext"][$p]).(isset($_GET["boolean"][$p])?" IN BOOLEAN MODE":"").")";}foreach((array)$_GET["where"]as$Z){$_b=$Z["col"];$Eh=$Z["op"];$X=$Z["val"];if("$_b$X"!=""&&in_array($Eh,$this->getOperators())){$Rb=[];foreach(($_b!=""?[$_b=>$k[$_b]]:$k)as$A=>$j){$Ki="";$Qb=" $Eh";$uh=DIALECT=="pgsql"&&$Eh=="="&&$j["type"]=="oid";if($uh)$Qb
.=" ".$this->admin->processFieldInput($j,$X)."::regproc";elseif(preg_match('~IN$~',$Eh)){$Ze=process_length($X);$Qb
.=" ".($Ze!=""?$Ze:"(NULL)");}elseif($Eh=="SQL")$Qb=" $X";elseif(preg_match('~^(I?LIKE) %%$~',$Eh,$y))$Qb=" $y[1] ".$this->admin->processFieldInput($j,"%$X%");elseif($Eh=="FIND_IN_SET"){$Ki="$Eh(".q($X).", ";$Qb=")";}elseif(!preg_match('~NULL$~',$Eh))$Qb
.=" ".$this->admin->processFieldInput($j,$X);if($_b!=""||(isset($j["privileges"]["where"])&&(preg_match('~^[-\d.'.(preg_match('~IN$~',$Eh)?',':'').']+$~',$X)||!preg_match('~'.number_type().'|bit~',$j["type"]))&&(!preg_match("~[\x80-\xFF]~",$X)||preg_match('~char|text|enum|set~',$j["type"]))&&(!preg_match('~date|timestamp~',$j["type"])||preg_match('~^\d+-\d+-\d+~',$X))&&(!preg_match('~^elastic~',DRIVER)||$j["type"]!="boolean"||preg_match('~true|false~',$X))&&(!preg_match('~^elastic~',DRIVER)||strpos($Eh,"regexp")===false||preg_match('~text|keyword~',$j["type"])))){if($uh)$Rb[]=$Ki.idf_escape($A).$Qb;else$Rb[]=$Ki.Driver::get()->convertSearch(idf_escape($A),$Z,$j).$Qb;}}if(count($Rb)==1)$J[]=$Rb[0];elseif($Rb)$J[]="(".implode(" OR ",$Rb).")";else$J[]="1 = 0";}}return$J;}function
processSelectionOrder(array$k,array$s){$J=[];foreach((array)$_GET["order"]as$t=>$X){if($X!="")$J[]=(preg_match('~^((COUNT\(DISTINCT |[A-Z0-9_]+\()(`(?:[^`]|``)+`|"(?:[^"]|"")+")\)|COUNT\(\*\))$~',$X)?$X:idf_escape($X)).(isset($_GET["desc"][$t])?" DESC":"");}return$J;}function
processSelectionLength(){return
isset($_GET["text_length"])?$_GET["text_length"]:"100";}function
getFieldFunctions(array$j){$J=($j["null"]?"NULL/":"");$Zl=isset($_GET["select"])||where($_GET);foreach([Driver::get()->getInsertFunctions(),Driver::get()->getEditFunctions()]as$t=>$pe){if(!$t||(!isset($_GET["call"])&&$Zl)){foreach($pe
as$vi=>$X){if(!$vi||preg_match("~$vi~",$j["type"]))$J
.="/$X";}}if($t&&$pe&&!preg_match('~enum|set|bool~',$j["type"])&&!is_blob($j))$J
.="/SQL";}if($j["auto_increment"]&&!$Zl)$J=lang(47);return
explode("/",$J);}function
getFieldInput($Q,array$j,$Ma,$Y,$o){return"";}function
processFieldInput(array$j,$Y,$o=""){if($o=="SQL")return$Y;if(isset($j["full_type"]))$this->admin->detectJson($j["full_type"],$Y,false);$A=$j["field"];$J=q($Y);if(preg_match('~^(now|getdate|uuid)$~',$o))$J="$o()";elseif(preg_match('~^current_(date|timestamp)$~',$o))$J=$o;elseif(preg_match('~^([+-]|\|\|)$~',$o))$J=idf_escape($A)." $o $J";elseif(preg_match('~^[+-] interval$~',$o))$J=idf_escape($A)." $o ".(preg_match("~^(\\d+|'[0-9.: -]') [A-Z_]+\$~i",$Y)&&DIALECT!="pgsql"?$Y:$J);elseif(preg_match('~^(addtime|subtime|concat)$~',$o))$J="$o(".idf_escape($A).", $J)";elseif(preg_match('~^(md5|sha1|password|encrypt)$~',$o))$J="$o($J)";elseif($j["type"]=="boolean"&&DIALECT=="elastic")$J=$J=="0"?"false":"true";return
unconvert_field($j,$J);}function
getDumpOutputs(){$ci=['file'=>lang(67),'text'=>lang(68),];if(function_exists('gzencode'))$ci['gz']='gzip';return$ci;}function
getDumpFormats(){return(support("dump")?['sql'=>'SQL']:[])+['csv'=>'CSV,','csv;'=>'CSV;','tsv'=>'TSV'];}function
sendDumpHeaders($Ue,$Vg=false){$bi=$_POST["output"];$Cd=(str_contains($_POST["format"],"sql")?"sql":($Vg?"tar":"csv"));if($bi=="gz"){header("Content-Type: application/x-gzip");ob_start(function($Ek){return
gzencode($Ek);},1e6);}elseif($Cd=="tar")header("Content-Type: application/x-tar");elseif($Cd=="sql"||$bi=="text")header("Content-Type: text/plain; charset=utf-8");else
header("Content-Type: text/csv; charset=utf-8");return$Cd;}function
dumpTable($Q,$Hk,$tm=0){if($_POST["format"]!="sql"){echo"\xef\xbb\xbf";if($Hk)dump_csv(array_keys(fields($Q)));}else{if($tm==2){$k=[];foreach(fields($Q)as$A=>$j)$k[]=idf_escape($A)." $j[full_type]";$cc="CREATE TABLE ".table($Q)." (".implode(", ",$k).")";}else$cc=create_sql($Q,$_POST["auto_increment"],$Hk);set_utf8mb4($cc);if($Hk&&$cc){if($Hk=="DROP+CREATE"||$tm==1)echo"DROP ".($tm==2?"VIEW":"TABLE")." IF EXISTS ".table($Q).";\n";if($tm==1)$cc=remove_definer($cc);echo"$cc;\n\n";}}}function
dumpData($Q,$Hk,$H){if($Hk){$xg=(DIALECT=="sqlite"?0:1048576);$k=[];$Ve=false;if($_POST["format"]=="sql"){if($Hk=="TRUNCATE+INSERT")echo
truncate_sql($Q).";\n";$k=fields($Q);if(DIALECT=="mssql"){foreach($k
as$j){if($j["auto_increment"]){echo"SET IDENTITY_INSERT ".table($Q)." ON;\n";$Ve=true;break;}}}}$I=Connection::get()->query($H,1);if($I){$kf="";$eb="";$Jf=[];$re=[];$Kk="";$bc=0;while($K=($Q!=''?$I->fetchAssoc():$I->fetchRow())){if(!$Jf){$nm=[];foreach($K
as$X){$j=$I->fetchField();if(!empty($k[$j->name]['generated'])){$re[$j->name]=true;continue;}$Jf[]=$j->name;$t=idf_escape($j->name);$nm[]="$t = VALUES($t)";}$Kk=($Hk=="INSERT+UPDATE"?"\nON DUPLICATE KEY UPDATE ".implode(", ",$nm):"").";\n";}if($_POST["format"]!="sql"){if($Hk=="table"){dump_csv($Jf);$Hk="INSERT";}dump_csv($K);}else{if(!$kf)$kf="INSERT INTO ".table($Q)." (".implode(", ",array_map('AdminNeo\idf_escape',$Jf)).") VALUES";foreach($K
as$t=>$X){if(isset($re[$t])){unset($K[$t]);continue;}$j=$k[$t];$K[$t]=($X===null?"NULL":($X===false?0:unconvert_field($j,preg_match(number_type(),$j["type"])&&!preg_match('~\[~',$j["full_type"])&&is_numeric($X)?$X:(!is_blob($j)||is_utf8($X)?q($X):Driver::get()->quoteBinary($X)))));}$Dj=($xg?"\n":" ")."(".implode(",\t",$K).")";if(!$eb)$eb=$kf.$Dj;elseif(DIALECT=="mssql"?$bc%1000!=0:strlen($eb)+4+strlen($Dj)+strlen($Kk)<$xg)$eb
.=",$Dj";else{echo$eb.$Kk;$eb=$kf.$Dj;}}$bc++;}if($eb)echo$eb.$Kk;}elseif($_POST["format"]=="sql")echo"-- ".str_replace("\n"," ",Connection::get()->getError())."\n";if($Ve)echo"SET IDENTITY_INSERT ".table($Q)." OFF;\n";}}function
getImportFilePath(){return"adminneo.sql";}function
printDatabaseMenu(){echo"<p class='links top-links'>\n";$nh=isset($_GET["ns"])?$_GET["ns"]:null;if($nh==""&&support("database"))echo'<a href="',h(ME),'database=">',icon("edit"),lang(69),"</a>\n";if($nh!=""&&support("scheme"))echo"<a href='",h(ME),"scheme='>",icon("edit"),lang(70),"</a>\n";if($nh!=="")echo'<a href="',h(ME),'schema=">',icon("schema"),lang(71),"</a>\n";if(support("privileges"))echo"<a href='",h(ME),"privileges='>",icon("users"),lang(72),"</a>\n";echo"</p>\n";}function
printNavigation($Pg){parent::printNavigation($Pg);if($Pg=="auth"){$bi="";foreach((array)$_SESSION["pwds"]as$pm=>$hk){foreach($hk
as$N=>$jm){foreach($jm
as$V=>$F){if($F!==null){$rc=$_SESSION["db"][$pm][$N][$V];foreach(($rc?array_keys($rc):[""])as$g){$ek=$this->admin->getServerName($N,false);$T=h(get_driver_name($pm,$N)).($V!=""||$ek!=""?" - ":"").h($V).($V!=""&&$ek!=""?"@":"").h($ek).($g!=""?h(" - $g"):"");$bi
.="<li><a href='".h(auth_url($pm,$N,$V,$g))."' class='primary' title='$T'>$T</a></li>\n";}}}}}if($bi)echo"<nav id='logins'><menu>\n$bi</menu></nav>\n";}else{$this->admin->printDatabaseSwitcher($Pg);$va=[];if(DB==""||!$Pg){if(support("sql")){$va[]="<a href='".h(ME)."sql='".bold(isset($_GET["sql"])&&!isset($_GET["import"])).">".icon("command").lang(40)."</a>";$va[]="<a href='".h(ME)."import='".bold(isset($_GET["import"])).">".icon("import").lang(73)."</a>";}$va[]="<a href='".h(ME)."dump=".urlencode(isset($_GET["table"])?$_GET["table"]:$_GET["select"])."' id='dump'".bold(isset($_GET["dump"])).">".icon("export").lang(74)."</a>";}if(DB=="")$va[]='<a href="'.h(ME).'database="'.bold($_GET["database"]==="").">".icon("database-add").lang(75)."</a>\n";if(DB!=""&&$_GET["ns"]===""&&!$Pg)$va[]='<a href="'.h(ME).'scheme="'.bold($_GET["scheme"]==="").">".icon("database-add").lang(76)."</a>\n";if(DB!=""&&$_GET["ns"]!==""&&!$Pg)$va[]='<a href="'.h(ME).'create="'.bold($_GET["create"]==="").">".icon("table-add").lang(77)."</a>\n";if($va)echo"<p class='links'>".implode("\n",$va)."</p>";$S=[];if($_GET["ns"]!==""&&!$Pg&&DB!=""){Connection::get()->selectDatabase(DB);$S=table_status('',true);}if($_GET["ns"]!==""&&!$Pg&&DB!=""){if($S){$this->admin->printTablesFilter();$this->admin->printTableList($S);}else
echo"<p class='message'>".lang(78)."</p>\n";}if(support("sql")||DIALECT=="elastic"||DIALECT=="mongo"){echo"<script".nonce().">\n";if(support("sql")&&$S){$jg=[];foreach($S
as$Q=>$U)$jg[]=js_escape_re($Q);$Vk=support("table")&&!$this->config->isSelectionPreferred()?"table":"select";echo"window.jushLinks = { ".DIALECT.": {\n",js_escape_key(ME.$Vk.'=$&'),': /\b(?<!\$)('.implode('|',$jg).')(?!\$)\b/g';if(support('routine')){foreach(routines()as$K)echo",\n",js_escape_key(ME.'function='.urlencode($K["SPECIFIC_NAME"]).'&name=$&'),': /\b'.js_escape_re($K["ROUTINE_NAME"]).'(?=["`\]]?\()/g';}echo"\n}};\n";foreach(["bac","bra","sqlite_quo","mssql_bra"]as$X)echo"jushLinks.$X = jushLinks.".DIALECT.";\n";}if(DIALECT!="elastic"&&DIALECT!="mongo"&&$this->getConfig()->isSqlAutocompletionEnabled()&&(isset($_GET["sql"])||isset($_GET["trigger"])||isset($_GET["check"]))){$fl=array_fill_keys(array_keys($S),[]);foreach(Driver::get()->getAllFields()as$Q=>$k){foreach($k
as$j)$fl[$Q][]=$j["field"];}echo"window.addEventListener('DOMContentLoaded', () => { autocompletion = jush.autocompleteSql('".idf_escape("")."', ".json_encode($fl,JSON_HEX_TAG)."); });\n";}echo"</script>\n";}echo
script("let autocompletion;\nwindow.addEventListener('DOMContentLoaded', () => { initSyntaxHighlighting('".js_escape(doc_version())."', '".js_escape(Connection::get()->getFlavor())."', autocompletion); });");}}function
printDatabaseSwitcher($Pg){$f=$this->admin->getDatabases();if(!$f&&DIALECT!="sqlite")return;echo"<div class='db-selector'><form action=''>";hidden_fields_get();echo"<div>";if($f)echo"<select id='database-select' name='db' title='",lang(30),"'>".optionlist([""=>"(".lang(79).")"]+$f,DB)."</select>".script("mixin(gid('database-select'), {onmousedown: dbMouseDown, onchange: dbChange});");else
echo"<input id='database-select' class='input' name='db' value='".h(DB)."' title='",lang(30),"' autocapitalize='off'>\n";echo"<input type='submit' value='".lang(80)."' class='button ".($f?"hidden":"")."'>\n","</div>";foreach(["import","sql","schema","dump","privileges"]as$X){if(isset($_GET[$X])){echo
input_hidden($X);break;}}echo"</form></div>\n";}function
printTableList(array$S){$Xc=$this->settings->isNavigationDual()||$this->settings->isNavigationHover();$Fg=($Xc?"class='dual".($this->settings->isNavigationHover()?" hover":"")."'":($this->settings->isNavigationReversed()?"class='reversed'":""));echo"<nav id='tables'><menu $Fg>";foreach($S
as$Q=>$P){$Q="$Q";$A=$this->admin->getTableName($P);if($A==""||(isset($P["Partition"])?$P["Partition"]:false))continue;echo"<li>";$wa=in_array($Q,[$_GET["table"],$_GET["select"],$_GET["create"],$_GET["indexes"],$_GET["foreign"],$_GET["trigger"],$_GET["check"],$_GET["view"]]);$yb="primary".(is_view($P)?" view":"");$Ok=support("table")||support("indexes");$Sj=h(ME)."select=".urlencode($Q);$Xk=h(ME)."table=".urlencode($Q);if($this->settings->isSelectionPreferred()){if($this->settings->isNavigationReversed()&&$Ok)echo" <a href='$Xk' title='",lang(34),"' class='secondary'>",icon("structure"),"</a>";echo"<a href='$Sj'",bold($wa,$yb)," data-primary='true' title='$A'>$A</a>";if($Xc&&$Ok)echo" <a href='$Xk' title='",lang(34),"' class='secondary'>",icon_solo("structure"),"</a>";}else{if($this->settings->isNavigationReversed())echo" <a href='$Sj' title='",lang(33),"' class='secondary'>",icon("data"),"</a>";if($Ok)echo"<a href='$Xk'",bold($wa,$yb)," data-primary='true' title='$A'>$A</a>";else
echo"<span data-primary='true'",bold($wa,$yb),">$A</span>";if($Xc)echo" <a href='$Sj' title='",lang(33),"' class='secondary'>",icon_solo("data"),"</a>";}echo"</li>\n";}echo"</menu></nav>\n",script("initTablesList(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");");}function
getSettingsRows($ze){$O=parent::getSettingsRows($ze);if($ze==1){$C=[""=>lang(14),Config::$NavigationSimple=>lang(81),Config::$NavigationDual=>lang(82),Config::$NavigationHover=>lang(83),Config::$NavigationReversed=>lang(84)];$h=$C[$this->config->getNavigationMode()];$C[""].=" ($h)";$O["navigationMode"]="<tr><th>".lang(85)."</th>"."<td>".html_radios("navigationMode",$C,($ra=$this->settings->getParameter("navigationMode"))!==null?$ra:"")."<span class='input-hint'>".lang(86)."</span>"."</td></tr>\n";$C=[""=>lang(14),0=>lang(34),1=>lang(33),];$h=$C[$this->config->isSelectionPreferred()?1:0];$C[""].=" ($h)";$O["preferSelection"]="<tr><th id='label-links'>".lang(87)."</th>"."<td>".html_select("preferSelection",$C,($ra=$this->settings->getParameter("preferSelection"))!==null?$ra:"","","label-links",true)."<span class='input-hint'>".lang(88)."</span>"."</td></tr>\n";}return$O;}function
getForeignColumnInfo(array$de,$b){return
null;}}class
TmpFile{private$handler;private$size;function
__construct(){$this->handler=tmpfile();}function
getSize(){return$this->size;}function
write($Xb){if(!$this->handler)return;$this->size+=strlen($Xb);fwrite($this->handler,$Xb);}function
send(){if(!$this->handler)return;fseek($this->handler,0);fpassthru($this->handler);fclose($this->handler);}}function
print_select_result(Result$I,$d=null,array$Th=[],$v=0){$jg=[];$s=[];$c=[];$ab=[];$Pl=[];$J=[];for($p=0;(!$v||$p<$v)&&($K=$I->fetchRow());$p++){if(!$p){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>";for($Cf=0;$Cf<count($K);$Cf++){$j=$I->fetchField();if(!$j){echo"<th></th>";continue;}$A=$j->name;$Sh=isset($j->orgtable)?$j->orgtable:"";$Rh=isset($j->orgname)?$j->orgname:$A;if(isset($j->table))$J[$j->table]=$Sh;if($Th&&DIALECT=="sql")$jg[$Cf]=($A=="table"?"table=":($A=="possible_keys"?"indexes=":null));elseif($Sh!=""){if(!isset($s[$Sh])){$s[$Sh]=[];foreach(indexes($Sh,$d)as$r){if($r["type"]=="PRIMARY"){$s[$Sh]=array_flip($r["columns"]);break;}}$c[$Sh]=$s[$Sh];}if(isset($c[$Sh][$Rh])){unset($c[$Sh][$Rh]);$s[$Sh][$Rh]=$Cf;$jg[$Cf]=$Sh;}}if($j->charsetnr==63)$ab[$Cf]=true;$Pl[$Cf]=$j->type;$T=trim(($Sh!=""?"$Sh.$Rh":($j->name!=$Rh?$Rh:""))." ".Driver::get()->getTypeName($j));echo"<th".($T!=""?" title='".h($T)."'":"").">".h($A).($Th?doc_link(['sql'=>"explain-output.html#explain_".strtolower($A),'mariadb'=>"reference/sql-statements/administrative-sql-statements/analyze-and-explain-statements/explain#columns-in-explain-...-select",]):"");}echo"</thead>\n";}echo"<tr>";foreach($K
as$t=>$X){$w="";if(isset($jg[$t])&&!$c[$jg[$t]]){if($Th&&DIALECT=="sql"){$Q=$K[array_search("table=",$jg)];$w=ME.$jg[$t].urlencode($Th[$Q]!=""?$Th[$Q]:$Q);}else{$w=ME."edit=".urlencode($jg[$t]);foreach($s[$jg[$t]]as$_b=>$Cf)$w
.="&where".urlencode("[".bracket_escape($_b)."]")."=".urlencode($K[$Cf]);}}$U=($ab[$t]?'blob':($Pl[$t]==254?'char':''));$j=['full_type'=>$U,'type'=>$U,];$X=select_value($X,$w,$j,null);$yb=$Pl[$t]<=9||$Pl[$t]==246?"class='number'":"";echo"<td $yb>$X</td>";}}if($p)echo"</table>\n</div>";else
echo"<p class='message'>".lang(89);echo"\n";return$J;}function
referencable_primary($Xj){$J=[];foreach(table_status('',true)as$Zk=>$Q){if($Zk!=$Xj&&fk_support($Q)){foreach(fields($Zk)as$j){if($j["primary"]){if($J[$Zk]){unset($J[$Zk]);break;}$J[$Zk]=$j;}}}}return$J;}function
textarea($A,$Y,$L=10,$Fb=80){echo"<textarea name='".h($A)."' rows='$L' cols='$Fb' class='sqlarea jush-".DIALECT."' spellcheck='false' wrap='off'>";if(is_array($Y)){foreach($Y
as$X)echo
h($X[0])."\n\n\n";}else
echo
h($Y);echo"</textarea>";}function
select_input($Ma,$C,$Y="",$Ch="",$yi=""){$jl=($C?"select":"input");return"<$jl $Ma".($C?"><option value=''>$yi".optionlist($C,$Y,true)."</select>":" size='10' value='".h($Y)."' placeholder='$yi'>").($Ch?script("qsl('$jl').onchange = $Ch;",""):"");}function
json_row($t,$X=null){static$Wd=true;if($Wd)echo"{";if($t!=""){echo($Wd?"":",")."\n\t\"".addcslashes($t,"\r\n\t\"\\/").'": '.($X!==null?'"'.addcslashes($X,"\r\n\t\"\\/").'"':'null');$Wd=false;}else{echo"\n}\n";$Wd=true;}}function
edit_type($t,$j,$Cb,$ee=[],$Fd=[]){$U=isset($j["type"])?$j["type"]:null;echo'<td><select name="',h($t),'[type]" class="type" aria-labelledby="label-type">';$Rc=Driver::get()->getTypes();if($U&&!isset($Rc[$U])&&!isset($ee[$U])&&!in_array($U,$Fd))$Fd[]=$U;$Gk=Driver::get()->getStructuredTypes();if($ee)$Gk[lang(90)]=$ee;echo
optionlist(array_merge($Fd,$Gk),$U),'</select><td><input name="',h($t),'[length]" value="',h(isset($j["length"])?$j["length"]:null),'" size="3"',(!(isset($j["length"])?$j["length"]:null)&&preg_match('~var(char|binary)$~',$U)?" class='input required'":" class='input'"),' aria-labelledby="label-length"><td class="options">',($Cb?"<select name='".h($t)."[collation]'".(preg_match('~(char|text|enum|set)$~',$U)?"":" class='hidden'").'><option value="">('.lang(91).')'.optionlist($Cb,isset($j["collation"])?$j["collation"]:null).'</select>':''),(Driver::get()->getUnsigned()?"<select name='".h($t)."[unsigned]'".(!$U||preg_match(number_type(),$U)?"":" class='hidden'").'><option>'.optionlist(Driver::get()->getUnsigned(),isset($j["unsigned"])?$j["unsigned"]:null).'</select>':''),(isset($j['on_update'])?"<select name='".h($t)."[on_update]'".(preg_match('~timestamp|datetime~',$U)?"":" class='hidden'").'>'.optionlist([""=>"(".lang(92).")","CURRENT_TIMESTAMP"],(preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?"CURRENT_TIMESTAMP":$j["on_update"])).'</select>':''),($ee?"<select name='".h($t)."[on_delete]'".(preg_match("~`~",$U)?"":" class='hidden'")."><option value=''>(".lang(93).")".optionlist(Driver::get()->getOnActions(),isset($j["on_delete"])?$j["on_delete"]:null)."</select> ":" ");}function
process_length($u){$nd=Driver::$EnumLengthPattern;return(preg_match("~^\\s*\\(?\\s*$nd(?:\\s*,\\s*$nd)*+\\s*\\)?\\s*\$~",$u)&&preg_match_all("~$nd~",$u,$z)?"(".implode(",",$z[0]).")":preg_replace('~^[0-9].*~','(\0)',preg_replace('~[^-0-9,+()[\]]~','',$u)));}function
process_type($j,$Ab="COLLATE"){return" $j[type]".process_length($j["length"]).(preg_match(number_type(),$j["type"])&&in_array($j["unsigned"],Driver::get()->getUnsigned())?" $j[unsigned]":"").(preg_match('~char|text|enum|set~',$j["type"])&&$j["collation"]?" $Ab ".(DIALECT=="mssql"?$j["collation"]:q($j["collation"])):"");}function
process_field($j,$Nl){if($j["on_update"])$j["on_update"]=preg_replace('~current_timestamp(\(\))?~i',"CURRENT_TIMESTAMP",$j["on_update"]);return[idf_escape(trim($j["field"])),process_type($Nl),($j["null"]?" NULL":" NOT NULL"),default_value($j),(preg_match('~timestamp|datetime~',$j["type"])&&$j["on_update"]?" ON UPDATE ".$j["on_update"]:""),(support("comment")&&$j["comment"]!=""?" COMMENT ".q(normalize_newlines($j["comment"])):""),($j["auto_increment"]?auto_increment():null),];}function
normalize_newlines($Y){return
str_replace("\r","",(string)$Y);}function
default_value($j){if($j["default"]===null)return"";$h=normalize_newlines($j["default"]);$qe=$j["generated"];if(in_array($qe,Driver::get()->getGenerated())){if(DIALECT=="mssql")return" AS ($h)".($qe=="VIRTUAL"?"":" $qe");else
return" GENERATED ALWAYS AS ($h) $qe";}if(stripos($h,"GENERATED ")===0)return" $h";if(preg_match('~char|binary|text|json|enum|set~',$j["type"])||preg_match('~^(?![a-z])~i',$h)){if(DIALECT=="sql"&&preg_match('~text|json~',$j["type"]))return" DEFAULT (".q($h).")";else
return" DEFAULT ".q($h);}else{$h=str_ireplace("current_timestamp()","CURRENT_TIMESTAMP",$h);return" DEFAULT ".(DIALECT=="sqlite"?"($h)":$h);}}function
type_class($U){foreach(['char'=>'text','date'=>'time|year','binary'=>'blob','enum'=>'set',]as$yb=>$vi){if(preg_match("~$yb|$vi~",$U))return"class='$yb'";}return"";}function
edit_fields(array$k,array$Cb,$U="TABLE",$ee=[]){$k=array_values($k);$Nb=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Lb=$Nb?"":"class='hidden'";echo"<thead><tr>\n";if(support("move_col"))echo"<td class='jsonly'></td>";if($U=="PROCEDURE")echo"<td></td>";echo"<th id='label-name'>",($U=="TABLE"?lang(94):lang(95)),"</th>\n","<td id='label-type'>",lang(44),"<textarea id='enum-edit' rows='4' cols='12' wrap='off' hidden></textarea>",script("gid('enum-edit').onblur = onFieldLengthBlur;"),"</td>\n","<td id='label-length'>",lang(96),"</td>\n","<td>",lang(97),"</td>\n";if($U=="TABLE")echo"<td id='label-null'>NULL</td>\n","<td><input type='radio' name='auto_increment_col' value=''><abbr id='label-ai' title='",lang(47),"'>AI</abbr>",doc_link(['sql'=>"example-auto-increment.html",'mariadb'=>"reference/data-types/auto_increment",]),"</td>\n","<td id='label-default'>",lang(48),"</td>\n",support("comment")?"<td id='label-comment' $Lb>".lang(46)."</td>\n":"";echo"<td>","<button name='add[",(support("move_col")?0:count($k)),"]' value='1' title='",lang(98),"' class='button light'>",icon_solo("add"),"</button>",(support("move_col")?"":script("qsl('button').onclick = onAddLastFieldRowClick;")),script("row_count = ".count($k).";"),"</td>\n","</tr></thead>\n";$yb=support("move_col")?"class='sortable'":"";echo"<tbody $yb>\n";foreach($k
as$p=>$j){$p++;$Uh=$j[($_POST?"orig":"field")];$Hc=(isset($_POST["add"][$p-1])||(isset($j["field"])&&!(isset($_POST["drop_col"][$p])?$_POST["drop_col"][$p]:null)))&&(support("drop_col")||$Uh=="");echo"<tr",($Hc?"":" hidden"),">\n";if(support("move_col"))echo"<td class='handle jsonly'>",icon_solo("handle"),"</td>";if($U=="PROCEDURE")echo"<td>",html_select("fields[$p][inout]",Driver::get()->getInOut(),$j["inout"]),"</td>\n";echo"<th>";if($Hc)echo"<input class='input' name='fields[$p][field]' value='",h($j["field"]),"' data-maxlength='64' autocapitalize='off' aria-labelledby='label-name' ".(isset($_POST["add"][$p-1])?"autofocus":"").">";echo
input_hidden("fields[$p][orig]",$Uh);edit_type("fields[$p]",$j,$Cb,$ee);echo"</th>\n";if($U=="TABLE"){echo"<td>",checkbox("fields[$p][null]",1,$j["null"],"","","block","label-null"),"</td>\n";$tb=$j["auto_increment"]?"checked":"";echo"<td><label class='block'><input type='radio' name='auto_increment_col' value='$p' $tb aria-labelledby='label-ai'></label></td>\n","<td class='default-value'>";if(Driver::get()->getGenerated())echo
html_select("fields[$p][generated]",array_merge(["","DEFAULT"],Driver::get()->getGenerated()),$j["generated"]);else
echo
checkbox("fields[$p][generated]",1,$j["generated"],"","","","label-default");$Ma="name='fields[$p][default]' aria-labelledby='label-default'";$Y=h($j["default"]);if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";if(support("comment")){$wg=Connection::get()->isMinVersion("5.5")?1024:255;$Ma="name='fields[$p][comment]' data-maxlength='$wg' aria-labelledby='label-comment'";$Y=h($j["comment"]);echo"<td $Lb>";if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";}}echo"<td>";if(support("move_col"))echo"<button name='add[$p]' value='1' title='".lang(98)."' class='button light'>",icon_solo("add"),"</button>","<button name='up[$p]' value='1' title='".lang(99)."' class='button light hidden'>",icon_solo("arrow-up"),"</button>","<button name='down[$p]' value='1' title='".lang(100)."' class='button light hidden'>",icon_solo("arrow-down"),"</button>";if($Uh==""||support("drop_col"))echo"<button name='drop_col[$p]' value='1' title='".lang(58)."' class='button light'>",icon_solo("remove"),"</button>";echo"</td>\n</tr>\n";}echo"</tbody>";}function
process_fields(&$k){$sh=0;if($_POST["up"]){$Uf=0;foreach($k
as$t=>$j){if(key($_POST["up"])==$t){unset($k[$t]);array_splice($k,$Uf,0,[$j]);break;}if(isset($j["field"]))$Uf=$sh;$sh++;}}elseif($_POST["down"]){$je=false;foreach($k
as$t=>$j){if(isset($j["field"])&&$je){unset($k[key($_POST["down"])]);array_splice($k,$sh,0,[$je]);break;}if(key($_POST["down"])==$t)$je=$j;$sh++;}}elseif($_POST["add"]){$k=array_values($k);array_splice($k,key($_POST["add"]),0,[[]]);}elseif(!$_POST["drop_col"])return
false;return
true;}function
normalize_enum($y){$X=$y[0];return"'".str_replace("'","''",addcslashes(stripcslashes(str_replace($X[0].$X[0],$X[0],substr($X,1,-1))),'\\'))."'";}function
grant($ue,array$Ri,$c,$Ah,$hm){if(!$Ri)return
true;if($Ri==["ALL PRIVILEGES","GRANT OPTION"]){if($ue)return(bool)queries("GRANT ALL PRIVILEGES ON $Ah TO $hm WITH GRANT OPTION");else
return
queries("REVOKE ALL PRIVILEGES ON $Ah FROM $hm")&&queries("REVOKE GRANT OPTION ON $Ah FROM $hm");}if($Ri==["GRANT OPTION","PROXY"]){if($ue)return(bool)queries("GRANT PROXY ON $Ah TO $hm WITH GRANT OPTION");else
return(bool)queries("REVOKE PROXY ON $Ah FROM $hm");}return(bool)queries(($ue?"GRANT ":"REVOKE ").preg_replace('~(GRANT OPTION)\([^)]*\)~','$1',implode("$c, ",$Ri).$c)." ON $Ah ".($ue?"TO ":"FROM ").$hm);}function
drop_create($Tc,$cc,$Uc,$ol,$Vc,$x,$Ig,$Gg,$Hg,$zh,$ih){if($_POST["drop"])query_redirect($Tc,$x,$Ig);elseif($zh=="")query_redirect($cc,$x,$Hg);elseif($zh!=$ih){$fc=queries($cc);queries_redirect($x,$Gg,$fc&&queries($Tc));if($fc)queries($Uc);}else
queries_redirect($x,$Gg,queries($ol)&&queries($Vc)&&queries($Tc)&&queries($cc));}function
create_trigger($Ah,array$Il){$xl=" $Il[Timing] $Il[Event]".(preg_match('~ OF~',$Il["Event"])?" $Il[Of]":"");return"CREATE TRIGGER ".idf_escape($Il["Trigger"]).(DIALECT=="mssql"?$Ah.$xl:$xl.$Ah).rtrim(" $Il[Type]\n$Il[Statement]",";").";";}function
create_routine($_j,$K){$kk=[];$k=(array)$K["fields"];ksort($k);$af=implode("|",Driver::get()->getInOut());foreach($k
as$j){if($j["field"]!="")$kk[]=(preg_match("~^($af)\$~",$j["inout"])?"$j[inout] ":"").idf_escape($j["field"]).process_type($j,"CHARACTER SET");}$xc=rtrim($K["definition"],";");return"CREATE $_j ".idf_escape(trim($K["name"]))." (".implode(", ",$kk).")".($_j=="FUNCTION"?" RETURNS".process_type($K["returns"],"CHARACTER SET"):"").($K["language"]?" LANGUAGE $K[language]":"").(DIALECT=="pgsql"?" AS ".q($xc):"\n$xc;");}function
remove_definer($H){return
preg_replace('~^([A-Z =]+) DEFINER=`'.preg_replace('~@(.*)~','`@`(%|\1)',logged_user()).'`~','\1',$H);}function
format_foreign_key($n){$Bh=implode("|",Driver::get()->getOnActions());$g=$n["db"];$nh=$n["ns"];return" FOREIGN KEY (".implode(", ",array_map('AdminNeo\idf_escape',$n["source"])).") REFERENCES ".($g!=""&&$g!=$_GET["db"]?idf_escape($g).".":"").($nh!=""&&$nh!=$_GET["ns"]?idf_escape($nh).".":"").idf_escape($n["table"])." (".implode(", ",array_map('AdminNeo\idf_escape',$n["target"])).")".(preg_match("~^($Bh)\$~",$n["on_delete"])?" ON DELETE $n[on_delete]":"").(preg_match("~^($Bh)\$~",$n["on_update"])?" ON UPDATE $n[on_update]":"").(isset($n["deferrable"])?" $n[deferrable]":"");}function
tar_file($m,TmpFile$_l){$Ie=pack("a100a8a8a8a12a12",$m,644,0,0,decoct($_l->getSize()),decoct(time()));$vb=8*32;for($p=0;$p<strlen($Ie);$p++)$vb+=ord($Ie[$p]);$Ie
.=sprintf("%06o",$vb)."\0 ";echo$Ie,str_repeat("\0",512-strlen($Ie));$_l->send();echo
str_repeat("\0",511-($_l->getSize()+511)%512);}function
doc_link(array$ui,$pl="<sup>?</sup>"){if(!(isset($ui[DIALECT])?$ui[DIALECT]:null))return"";$qm=doc_version();$cm=['sql'=>"https://dev.mysql.com/doc/refman/$qm/en/",'sqlite'=>"https://www.sqlite.org/",'pgsql'=>"https://www.postgresql.org/docs/".(Connection::get()->isCockroachDB()?"current":$qm)."/",'mssql'=>"https://learn.microsoft.com/en-us/sql/",'oracle'=>"https://www.oracle.com/pls/topic/lookup?ctx=db".str_replace(".","",$qm)."&id=",'elastic'=>"https://www.elastic.co/guide/en/elasticsearch/reference/$qm/",];if(Connection::get()->isMariaDB()){$cm['sql']="https://mariadb.com/docs/server/";$ui['sql']=isset($ui['mariadb'])?$ui['mariadb']:str_replace(".html","",$ui['sql']);}return"<a href='".h($cm[DIALECT].$ui[DIALECT].(DIALECT=='mssql'?"?view=sql-server-ver$qm":""))."'".target_blank().">$pl</a>";}function
doc_version(){return
preg_replace('~^(\d\.?\d).*~s','\1',Connection::get()->getVersion());}function
db_size($g){if(!Connection::get()->selectDatabase($g))return"?";$J=0;foreach(table_status()as$R)$J+=$R["Data_length"]+$R["Index_length"];return
format_number($J);}function
set_utf8mb4($cc){static$kk=false;if(!$kk&&preg_match('~\butf8mb4~i',$cc)){$kk=true;echo"SET NAMES ".charset(Connection::get()).";\n\n";}}error_reporting(E_ALL&~E_DEPRECATED);set_error_handler(function($pd,$i){return(bool)preg_match('~^Undefined (array key|offset|index)~',$i);},E_WARNING|E_NOTICE);;$Td=!preg_match('~^(unsafe_raw)?$~',ini_get("filter.default"));if($Td||ini_get("filter.default_flags")){foreach(['_GET','_POST','_COOKIE','_SERVER']as$X){$Wl=filter_input_array(constant("INPUT$X"),FILTER_UNSAFE_RAW);if($Wl)$$X=$Wl;}}if(function_exists("mb_internal_encoding"))mb_internal_encoding("8bit");class
Server{private$params;private$key;function
__construct(array$gi,$t=null){$this->params=$gi;$this->key=$t;}function
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
getConfigParams(){$gi=isset($this->params["config"])?$this->params["config"]:[];$te=["servers"];foreach($te
as$fi){if(isset($gi[$fi]))unset($gi[$fi]);}return$gi;}}class
Config{static$NavigationSimple="simple";static$NavigationDual="dual";static$NavigationHover="hover";static$NavigationReversed="reversed";private$params;private$servers=[];function
__construct(array$gi){$this->params=$gi;if(isset($this->params["servers"])){foreach($this->params["servers"]as$t=>$N){$ck=new
Server($N,is_string($t)?$t:null);$this->params["servers"][$t]=$ck;$this->servers[$ck->getKey()]=$ck;}}}function
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
getDefaultDriver(array$Sc){$Qc=isset($this->params["defaultDriver"])?$this->params["defaultDriver"]:null;return$Qc&&isset($Sc[$Qc])?$Qc:key($Sc);}function
getDefaultServer(){$N=isset($this->params["defaultServer"])?$this->params["defaultServer"]:null;if($N===null)return
null;$ck=isset($this->params["servers"][$N])?$this->params["servers"][$N]:null;if($ck)return$ck->getKey();return$N;}function
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
getServerPairs(array$Sc){$qk=null;foreach($this->servers
as$N){if(!isset($Sc[$N->getDriver()]))continue;if(!$qk)$qk=$N->getDriver();elseif($N->getDriver()!=$qk){$qk=null;break;}}$dk=[];foreach($this->servers
as$t=>$N){if(!isset($Sc[$N->getDriver()]))continue;$bk=$N->getName();if($qk&&$bk)$dk[$t]=$bk;else$dk[$t]=$Sc[$N->getDriver()].($bk!=""?" - $bk":"");}return$dk;}function
getServer($ak){return
isset($this->servers[$ak])?$this->servers[$ak]:null;}function
applyServer($N){$N=$this->getServer($N);if(!$N)return;$this->params=array_merge($this->params,$N->getConfigParams());}private
function
parseList($lg){if(is_array($lg))return$lg;return
preg_split('~\s*,\s*~',(string)$lg);}}class
Settings{private
static$CookieName="neo_settings";static$ColorSchemeLight="light";static$ColorSchemeDark="dark";static$NavigationWidthMin=10;static$NavigationWidthMax=30;private$config;private$params=[];function
__construct(Config$Sb){$this->config=$Sb;if(isset($_COOKIE[self::$CookieName])){parse_str($_COOKIE[self::$CookieName],$this->params);$this->save();}if(isset($_COOKIE["neo_lang"])){$this->updateParameter("lang",$_COOKIE["neo_lang"]);unset($_COOKIE["neo_lang"]);cookie("neo_lang","",-3600);}}static
function
readParameter($t){parse_str(isset($_COOKIE[self::$CookieName])?$_COOKIE[self::$CookieName]:"",$gi);return
isset($gi[$t])?$gi[$t]:null;}function
getParameter($t,$h=null){return
isset($this->params[$t])?$this->params[$t]:$h;}function
updateParameter($t,$Y){$this->updateParameters([$t=>$Y]);}function
updateParameters(array$gi){$this->params=array_filter(array_merge($this->params,$gi),function($Y){return$Y!==null;});$this->save();}private
function
save(){cookie(self::$CookieName,http_build_query($this->params),7776000);}function
getColorScheme(){return$this->getParameter("colorScheme");}function
getNavigationMode(){return($ra=$this->getParameter("navigationMode"))!==null?$ra:$this->config->getNavigationMode();}function
isNavigationSimple(){return$this->getNavigationMode()==Config::$NavigationSimple;}function
isNavigationDual(){return$this->getNavigationMode()==Config::$NavigationDual;}function
isNavigationHover(){return$this->getNavigationMode()==Config::$NavigationHover;}function
isNavigationReversed(){return$this->getNavigationMode()==Config::$NavigationReversed;}function
getNavigationWidth(){$Fm=$this->getParameter("navigationWidth");if($Fm===null)return
null;return
min(max((float)$Fm,self::$NavigationWidthMin),self::$NavigationWidthMax);}function
isSelectionPreferred(){return($ra=$this->getParameter("preferSelection"))!==null?$ra:$this->config->isSelectionPreferred();}function
isRelationLinks(){return
isset($this->params["relationLinks"])?$this->params["relationLinks"]:$this->config->isRelationLinks();}function
getRecordsPerPage(){return($ra=$this->getParameter("recordsPerPage"))!==null?$ra:$this->config->getRecordsPerPage();}function
getEnumAsSelectThreshold(){$Y=$this->getParameter("enumAsSelectThreshold");if($Y<0)return
null;return$Y!==null?(int)$Y:$this->config->getEnumAsSelectThreshold();}}class
Hash{static
function
hkdf($u,$t,$ff="",$Ej=""){if(extension_loaded("hash")&&PHP_VERSION_ID>=70120)return
hash_hkdf("sha1",$t,$u,$ff,$Ej);if($Ej=="")$Ej=str_repeat("\0",20);$Si=self::hmacSha1($t,$Ej);$wh="";for($If="",$bb=1;!isset($wh[$u-1]);$bb++){$If=self::hmacSha1($If.$ff.chr($bb),$Si);$wh
.=$If;}return
substr($wh,0,$u);}static
function
hmacSha1($e,$t){if(!extension_loaded("hash"))return
hash_hmac("sha1",$e,$t,true);if(strlen($t)>64)$t=sha1($t,true);$t=str_pad($t,64,"\0");$uf=($t^str_repeat("\x36",64));$Fh=($t^str_repeat("\x5C",64));return
sha1($Fh.sha1($uf.$e,true),true);}}class
Random{static
function
strongKey(){return
strtr(rtrim(base64_encode(Random::bytes(32)),"="),"+/","-_");}static
function
bytes($u){if(PHP_VERSION_ID>=70000)return
random_bytes($u);$I=self::tryAlternatives($u);if($I!==false)return$I;$I=self::lastResortRandom($u);if($I!==false)return$I;throw
new
Exception("Error generating random bytes");}private
static
function
tryAlternatives($u){if(extension_loaded("libsodium"))return
\Sodium\randombytes_buf($u);$Vl=DIRECTORY_SEPARATOR==="/";if($Vl){$I=self::readDevUrandom($u);if($I!==false)return$I;}$fb=$Vl&&PHP_VERSION_ID>50609&&PHP_VERSION_ID<50613;if(extension_loaded("mcrypt")&&!$fb){$I=mcrypt_create_iv($u,MCRYPT_DEV_URANDOM);if($I!==false)return$I;}$gb=PHP_VERSION_ID<50444||(PHP_VERSION_ID>50500&&PHP_VERSION_ID<50528)||(PHP_VERSION_ID>50600&&PHP_VERSION_ID<50612);if(extension_loaded("openssl")&&!$gb){$I=openssl_random_pseudo_bytes($u,$Fk);if($Fk)return$I;}return
false;}private
static
function
readDevUrandom($u){static$l=null;if($l===null)$l=@fopen("/dev/urandom","rb");if(!$l)return
false;$pj=$u;$I="";do{$e=fread($l,$pj);if($e===false)return
false;$pj-=strlen($e);$I
.=$e;}while($pj>0);return$I;}private
static
function
readCapicom($u){$Hb=new
\COM("CAPICOM.Utilities.1");$pj=$u;$I="";do{$e=base64_decode((string)$Hb->GetRandom($u,0));$pj-=strlen($e);$I
.=$e;}while($pj>0);return$I;}private
static
function
lastResortRandom($u){static$t=null;static$Ej=null;if($t===null){$e=$_SERVER;$e[]=uniqid("",true);shuffle($e);$t=sha1(serialize($e),true);if(extension_loaded("openssl"))$Ej=openssl_random_pseudo_bytes(20);else{$Ej="";for($p=0;$p<20;$p++)$Ej
.=chr((mt_rand()^mt_rand())%256);}}else{if((ord($t)%2===0)===(ord($Ej)%2===0))$t=Hash::hmacSha1($t,$Ej);else$Ej=Hash::hmacSha1($Ej,$t);}return
Hash::hkdf($u,$t,"$u",$Ej);}}if(!function_exists("str_starts_with")){function
str_starts_with($He,$eh){return
strpos($He,$eh)===0;}}if(!function_exists("str_contains")){function
str_contains($He,$eh){return
strpos($He,$eh)!==false;}}if(!function_exists("password_verify")){function
password_verify($F,$Ge){return
false;}}if(!function_exists("ini_set")){function
ini_set($Lh,$Y){return
false;}}function
version(){return
VERSION;}function
idf_unescape($We){if(!preg_match('~^[`\'"[]~',$We))return$We;$Uf=substr($We,-1);return
str_replace($Uf.$Uf,$Uf,substr($We,1,-1));}function
q($Ek){return
Connection::get()->quote($Ek);}function
number($X){return
preg_replace('~[^0-9]+~','',$X);}function
number_type(){return'((?<!o)int(?!er)|numeric|real|float|double|decimal|money)';}function
remove_slashes(array$nm,$Td=false){$J=[];foreach($nm
as$t=>$X)$J[stripslashes($t)]=(is_array($X)?remove_slashes($X,$Td):($Td?$X:stripslashes($X)));return$J;}function
bracket_escape($We,$Ta=false){static$Fl=[':'=>':1',']'=>':2','['=>':3','"'=>':4'];return
strtr($We,($Ta?array_flip($Fl):$Fl));}function
min_version($qm,$tg=null,$d=null){if(!$d)$d=Connection::get();if($tg&&$d->isMariaDB())$qm=$tg;return$qm&&$d->isMinVersion($qm);}function
charset(Connection$d){return($d->isMinVersion("5.5.3")?"utf8mb4":"utf8");}function
link_files($A,array$Sd){switch($A){case'favicon-blue.ico':$m='favicon-blue-0f5ce53a66b1e25395d0048da369f19e__aff407a3.ico';break;case'favicon-blue.svg':$m='favicon-blue-17e440832c1eac07527560a0d6f0d2ee__aff407a3.svg';break;case'apple-touch-icon-blue.png':$m='apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__aff407a3.png';break;case'logo.svg':$m='logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg';break;case'jush.css':$m='jush-b3a93b18444da26820ff61746521dede__72e4fe51.css';break;case'jush-dark.css':$m='jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css';break;case'jush.js':$m='jush-615bc0b9720a1de8edd2c6876a3495b6__aab91337.js';break;case'icons.svg':$m='icons-70163a2695280bf75edba563e7b5471b__2ec7793c.svg';break;case'default-blue.css':$m='default-blue-564b3ff62703b0741b8754503c621af3__7018279f.css';break;case'default-blue-dark.css':$m='default-blue-dark-79895bd8e65cadab7d67d31c191a833d__7a7f64b1.css';break;case'main.js':$m='main-eaf2ce2c3d91edbef355936903e47e59__0b522a02.js';break;default:$m=null;break;}if(!$m)return
null;return
BASE_URL."?file=".urldecode($m);}function
ini_bool($Lh){$X=ini_get($Lh);return
preg_match('~^(on|true|yes)$~i',$X)||(int)$X;}function
ini_bytes($hf){$X=ini_get($hf);switch(strtolower(substr($X,-1))){case'g':$X=(int)$X*1024;case'm':$X=(int)$X*1024;case'k':$X=(int)$X*1024;}return$X;}function
sid(){static$J;if($J===null)$J=(session_id()&&!($_COOKIE&&ini_bool("session.use_cookies")));return$J;}function
save_driver_name($Qc,$N,$A){restart_session();$_SESSION["drivers"][$Qc][$N]=$A;stop_session();}function
get_driver_name($Qc,$N=null){return
isset($_SESSION["drivers"][$Qc][$N])?$_SESSION["drivers"][$Qc][$N]:Drivers::get($Qc);}function
save_login($Qc,$N,$V,$F,$g=""){$t=isset($_COOKIE["neo_key"])?$_COOKIE["neo_key"]:null;$_SESSION["pwds"][$Qc][$N][$V]=$t?[encrypt_string($F,$t)]:$F;$_SESSION["db"][$Qc][$N][$V][$g]=true;}function
delete_login($Qc,$N,$V){unset($_SESSION["pwds"][$Qc][$N][$V]);unset($_SESSION["db"][$Qc][$N][$V]);}function
get_password(){$F=get_session("pwds");if(is_array($F))return$_COOKIE["neo_key"]?decrypt_string($F[0],$_COOKIE["neo_key"]):false;return$F;}function
get_vals($H,$b=0){$J=[];$I=Connection::get()->query($H);if(is_object($I)){while($K=$I->fetchRow())$J[]=$K[$b];}return$J;}function
get_key_vals($H,$d=null,$lk=true){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchRow()){if($lk)$J[$K[0]]=$K[1];else$J[]=$K[0];}}return$J;}function
get_rows($H,$d=null,$i="<p class='error'>"){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchAssoc())$J[]=$K;}elseif(!$I&&!is_object($d)&&$i&&(defined("AdminNeo\PAGE_HEADER")||$i=="-- "))echo$i.error()."\n";return$J;}function
unique_array(array$K,array$s){foreach($s
as$r){if(!preg_match("~PRIMARY|UNIQUE~",$r["type"])&&!$r["partial"])continue;$Sl=[];foreach($r["columns"]as$t){if(!isset($K[$t]))continue
2;$Sl[$t]=$K[$t];}return$Sl;}return
null;}function
escape_key($t){if(preg_match('(^([\w(]+)('.str_replace("_",".*",preg_quote(idf_escape("_"))).')([ \w)]+)$)',$t,$y))return$y[1].idf_escape(idf_unescape($y[2])).$y[3];return
idf_escape($t);}function
where($Z,$k=[]){$Rb=[];foreach((array)$Z["where"]as$t=>$X){$t=bracket_escape($t,true);$b=escape_key($t);$Od=isset($k[$t]["type"])?$k[$t]["type"]:null;$ne=isset($k[$t]["full_type"])?$k[$t]["full_type"]:null;if(DIALECT=="sql"&&$Od=="json")$Rb[]="$b = CAST(".q($X)." AS JSON)";elseif(DIALECT=="pgsql"&&preg_match('~^jsonb?$~',$ne))$Rb[]="$b::jsonb = ".q($X)."::jsonb";elseif(DIALECT=="sql"&&is_numeric($X)&&strpos($X,".")!==false)$Rb[]="$b LIKE ".q($X);elseif(DIALECT=="mssql"&&strpos($Od,"datetime")===false)$Rb[]="$b LIKE ".q(preg_replace('~[_%[]~','[\0]',$X));else$Rb[]="$b = ".(isset($k[$t])?unconvert_field($k[$t],q($X)):q($X));if(DIALECT=="sql"&&preg_match('~char|text~',$Od)&&preg_match("~[^ -@]~",$X))$Rb[]="$b = ".q($X)." COLLATE ".charset(Connection::get())."_bin";}foreach((array)$Z["null"]as$t)$Rb[]=escape_key($t)." IS NULL";return
implode(" AND ",$Rb);}function
where_check($X,$k=[]){parse_str($X,$qb);remove_slashes([&$qb]);return
where($qb,$k);}function
where_link($p,$b,$Y,$Ih="="){return"&where%5B$p%5D%5Bcol%5D=".urlencode($b)."&where%5B$p%5D%5Bop%5D=".urlencode(($Y!==null?$Ih:"IS NULL"))."&where%5B$p%5D%5Bval%5D=".urlencode($Y);}function
convert_fields(array$c,array$k,array$M=[]){$I="";foreach($c
as$t=>$X){if($M&&!in_array(idf_escape($t),$M))continue;$La=convert_field($k[$t]);if($La)$I
.=", $La AS ".idf_escape($t);}return$I;}function
cookie_path(){return
strtr(preg_replace('~\?.*~','',$_SERVER["REQUEST_URI"]),[";"=>"%3B",","=>"%2C"]);}function
cookie($A,$Y,$eg=2592000){header("Set-Cookie: $A=".rawurlencode($Y).($eg?"; expires=".gmdate("D, d M Y H:i:s",time()+$eg)." GMT":"")."; path=".cookie_path().(HTTPS?"; secure":"")."; HttpOnly; SameSite=lax",false);}function
get_url($bm,$Yb){$J=@file_get_contents($bm,false,$Yb);if(function_exists('http_get_last_response_headers'))$http_response_header=($ra=http_get_last_response_headers())!==null?$ra:[];return[$J,isset($http_response_header)?$http_response_header:[]];}function
get_settings($ac="neo_settings"){parse_str(isset($_COOKIE[$ac])?$_COOKIE[$ac]:"",$O);return$O;}function
get_setting($t,$ac="neo_settings"){$O=get_settings($ac);return
isset($O[$t])?$O[$t]:null;}function
save_settings(array$O,$ac="neo_settings"){cookie($ac,http_build_query($O+get_settings($ac)));}function
restart_session(){if(!ini_bool("session.use_cookies")&&session_status()==PHP_SESSION_NONE)session_start();}function
stop_session($be=false){$fm=ini_bool("session.use_cookies");if(!$fm||$be){session_write_close();if($fm&&ini_set("session.use_cookies","0")===false)session_start();}}function&get_session($t){return$_SESSION[$t][DRIVER][SERVER][$_GET["username"]];}function
set_session($t,$X){$_SESSION[$t][DRIVER][SERVER][$_GET["username"]]=$X;}function
auth_url($pm,$N,$V,$g=null){$am=remove_from_uri(implode("|",array_keys(Drivers::getList()))."|username|ext|".($g!==null?"db|":"").($pm=='mssql'||$pm=='pgsql'?"":"ns|").session_name());preg_match('~([^?]*)\??(.*)~',$am,$y);return"$y[1]?".(sid()?session_name()."=".urlencode(session_id())."&":"").urlencode($pm)."=".urlencode($N)."&".($_GET["ext"]?"ext=".urlencode($_GET["ext"])."&":"")."username=".urlencode($V).($g!=""?"&db=".urlencode($g):"").($y[2]?"&$y[2]":"");}function
is_ajax(){return($_SERVER["HTTP_X_REQUESTED_WITH"]=="XMLHttpRequest");}function
redirect($x,$_=null){if($_!==null){restart_session();$_SESSION["messages"][preg_replace('~^[^?]*~','',($x!==null?$x:$_SERVER["REQUEST_URI"]))][]=$_;}if($x!==null){if($x=="")$x=".";header("Location: $x");exit;}}function
query_redirect($H,$x,$_,$gj=true,$wd=true,$Gd=false,$vl=""){if($wd){$Ak=microtime(true);$Gd=!Connection::get()->query($H);$vl=format_time($Ak);}$xk=$H?Admin::get()->formatMessageQuery($H,$vl,$Gd):"";if($Gd){Admin::get()->addError(error().$xk.script("initToggles();"));return
false;}if($gj)redirect($x,$_.$xk);return
true;}function
queries_redirect($x,$_,$gj){$Xi=implode("\n",Queries::$queries);$vl=format_time(Queries::$start);return
query_redirect($Xi,$x,$_,$gj,false,!$gj,$vl);}class
Queries{static$queries=[];static$start=0.0;}function
queries($H){if(!Queries::$start)Queries::$start=microtime(true);if(support("sql")){Queries::$queries[]=(preg_match('~;$~',$H)?"DELIMITER ;;\n$H;\nDELIMITER ":$H).";";return
Connection::get()->query($H);}else{Queries::$queries[]=$H;return[];}}function
apply_queries($H,array$S,$rd='AdminNeo\table'){foreach($S
as$Q){if(!queries("$H ".$rd($Q)))return
false;}return
true;}function
format_time($Ak){return
lang(101,max(0,microtime(true)-$Ak));}function
relative_uri(){return
str_replace(":","%3a",preg_replace('~^[^?]*/([^?]*)~','\1',$_SERVER["REQUEST_URI"]));}function
remove_from_uri($fi=""){return
substr(preg_replace("~(?<=[?&])($fi".(sid()?"":"|".session_name()).")=[^&]*&~",'',relative_uri()."&"),0,-1);}function
get_file($t,$sc=false,$zc=""){$l=$_FILES[$t];if(!$l)return
null;foreach($l
as$t=>$X)$l[$t]=(array)$X;$J='';foreach($l["error"]as$t=>$i){if($i)return$i;$A=$l["name"][$t];$Al=$l["tmp_name"][$t];$Wb=file_get_contents($sc&&preg_match('~\.gz$~',$A)?"compress.zlib://$Al":$Al);if($sc){$Ak=substr($Wb,0,3);if(function_exists("iconv")&&preg_match("~^\xFE\xFF|^\xFF\xFE~",$Ak))$Wb=iconv("utf-16","utf-8",$Wb);elseif($Ak=="\xEF\xBB\xBF")$Wb=substr($Wb,3);}if($zc){if(!preg_match("~$zc\\s*\$~",$Wb))$Wb
.=";";$Wb
.="\n\n";}$J
.=$Wb;}return$J;}function
upload_error($i){$_g=($i==UPLOAD_ERR_INI_SIZE?ini_get("upload_max_filesize"):0);return($i?lang(102).($_g?" ".lang(103,$_g):""):lang(104));}function
repeat_pattern($vi,$u){return
str_repeat("$vi{0,65535}",$u/65535)."$vi{0,".($u%65535)."}";}function
is_utf8($X){return(preg_match('~~u',$X)&&!preg_match('~[\0-\x8\xB\xC\xE-\x1F]~',$X));}function
format_number($X){return
strtr(number_format($X,0,".",lang(105)),preg_split('~~u',lang(106),-1,PREG_SPLIT_NO_EMPTY));}function
format_rows(array$R){$L=$R["Rows"];$Ia=($L&&(DIALECT=="sqlite"||(isset($R["Engine"])?$R["Engine"]:"")==(DIALECT=="pgsql"?"table":"InnoDB")));return($Ia?"~ ":"").format_number($L);}function
friendly_url($X){return
preg_replace('~\W~i','-',$X);}function
table_status1($Q,$Id=false){$J=table_status($Q,$Id);return($J?reset($J):["Name"=>$Q]);}function
column_foreign_keys($Q){$J=[];foreach(Admin::get()->getForeignKeys($Q)as$n){foreach($n["source"]as$X)$J[$X][]=$n;}return$J;}function
fields_from_edit(){$J=[];foreach((array)$_POST["field_keys"]as$t=>$X){if($X!=""){$X=bracket_escape($X);$_POST["function"][$X]=$_POST["field_funs"][$t];$_POST["fields"][$X]=$_POST["field_vals"][$t];}}foreach((array)$_POST["fields"]as$t=>$X){$A=bracket_escape($t,true);$J[$A]=["field"=>$A,"full_type"=>"varchar","type"=>"varchar","privileges"=>["insert"=>1,"update"=>1,"where"=>1,"order"=>1],"null"=>true,"auto_increment"=>($t==Driver::get()->primary),];}return$J;}function
dump_headers($Ue,$Wg=false){$Ue=friendly_url($Ue).date("-Ymd-His");$Cd=Admin::get()->sendDumpHeaders($Ue,$Wg);$bi=$_POST["output"];if($bi!="text")header("Content-Disposition: attachment; filename=$Ue.$Cd".($bi!="file"&&preg_match('~^[0-9a-z]+$~',$bi)?".$bi":""));session_write_close();if(!ob_get_level())ob_start(null,4096);ob_flush();flush();return$Cd;}function
dump_table_order(array$ch,array$mj){$Nf=array_flip($ch);$Ph=[];$ym=[];$kc=false;$xm=function($A)use(&$xm,&$Ph,&$ym,&$kc,$Nf,$mj){if(isset($Ph[$A]))return;if(isset($ym[$A])){$kc=true;return;}$ym[$A]=true;foreach(isset($mj[$A])?$mj[$A]:[]as$kj){if(isset($Nf[$kj]))$xm($kj);}unset($ym[$A]);$Ph[$A]=true;};foreach($ch
as$A)$xm($A);return($kc?null:array_keys($Ph));}function
dump_csv($K){$Ml=$_POST["format"]=="tsv";foreach($K
as$t=>$X){if(preg_match('~["\n]|^0[^.]|\.\d*0$|'.($Ml?'\t':'[,;]|^$').'~',$X))$K[$t]='"'.str_replace('"','""',$X).'"';}echo
implode(($_POST["format"]=="csv"?",":($Ml?"\t":";")),$K)."\r\n";}function
apply_sql_function($o,$b){return($o?($o=="unixepoch"?"DATETIME($b, '$o')":($o=="count distinct"?"COUNT(DISTINCT ":strtoupper("$o("))."$b)"):$b);}function
get_temp_dir(){$ti=ini_get("upload_tmp_dir");if(!$ti)$ti=sys_get_temp_dir();return$ti;}function
open_file_with_lock($m){if(is_link($m))return
null;$l=@fopen($m,"c+");if(!$l)return
null;@chmod($m,0660);if(!flock($l,LOCK_EX)){fclose($l);return
null;}return$l;}function
write_and_unlock_file($l,$e){rewind($l);fwrite($l,$e);ftruncate($l,strlen($e));unlock_file($l);}function
unlock_file($l){flock($l,LOCK_UN);fclose($l);}function
first(array$Ka){return
reset($Ka);}function
get_private_key($cc){$m=get_temp_dir()."/adminneo.key";if(!$cc&&!file_exists($m))return
false;$l=open_file_with_lock($m);if(!$l)return
false;$t=stream_get_contents($l);if(!$t){$t=Random::strongKey();write_and_unlock_file($l,$t);}else
unlock_file($l);return$t;}function
get_random_string(){return
Random::strongKey();}function
select_value($X,$w,$j,$rl){if(is_array($X)){$J="";if(array_filter($X,'is_array')==array_values($X)){$Jf=[];foreach($X
as$W)$Jf+=array_fill_keys(array_keys($W),null);foreach(array_keys($Jf)as$Ef)$J
.="<th>".h($Ef);foreach($X
as$W){$J
.="<tr>";foreach(array_merge($Jf,$W)as$km)$J
.="<td>".select_value($km,$w,$j,$rl);}}else{foreach($X
as$Ef=>$W)$J
.="<tr>".($X!=array_values($X)?"<th>".h($Ef):"")."<td>".select_value($W,$w,$j,$rl);}return"<table>$J</table>";}$Jj="";if($j&&$X!==null&&($rl===null||strlen($X)<=$rl)&&($nm=Driver::get()->explodeArrayValue($X,$j["full_type"],$Jj))){$Ij=$j;$Ij["type"]=$Ij["full_type"]=$Jj;$J=select_array_value($nm,$X,$w,$Ij,$rl);return
Driver::get()->implodeArrayValues($J,$j["full_type"]);}if(!$w)$w=Admin::get()->getFieldValueLink($X,$j);if($j)$X=Connection::get()->formatValue($X,$j);$J=$j?Admin::get()->formatFieldValue($X,$j):$X;if($J!==null){if(!is_utf8($J))$J="\0";elseif($rl!=""&&is_shortable($j))$J=truncate_utf8($J,max(0,+$rl));else$J=h($J);}return
Admin::get()->formatSelectionValue($J,$w,$j,$X);}function
select_array_value(array$nm,$X,$w,array$j,$rl){$I=[];foreach($nm
as$Y){if(is_array($Y))$I[]=select_array_value($Y,$X,$w,$j,$rl);else{$Of=preg_replace('~(where%5B\d+%5D%5Bval%5D=)'.preg_quote(urlencode($X),"~")."~",'${1}'.urlencode($Y),$w);$I[]=select_value($Y,$Of,$j,$rl);}}return$I;}function
is_blob(array$j){$Pl=Driver::get()->getStructuredTypes();$U=lang(107);return
preg_match('~blob|bytea|raw|file'.(DIALECT=="mssql"?'|binary|image':'').'~',$j["type"])&&!in_array($j["type"],isset($Pl[$U])?$Pl[$U]:[]);}function
is_mail($Y){return
is_string($Y)&&filter_var($Y,FILTER_VALIDATE_EMAIL);}function
is_web_url($Y){if(!is_string($Y)||!preg_match('~^(https?:)?//~i',$Y))return
false;$Ob=parse_url($Y);if(!$Ob)return
false;$bm=$Y;if(isset($Ob['path'])){$jd=array_map('urlencode',explode('/',$Ob['path']));$bm=str_replace($Ob['path'],implode('/',$jd),$bm);}if(isset($Ob['query'])){parse_str($Ob['query'],$gi);$bm=str_replace($Ob['query'],http_build_query($gi),$bm);}if(!isset($Ob['scheme']))$bm="https:$bm";return(bool)filter_var($bm,FILTER_VALIDATE_URL);}function
is_shortable($j){return$j&&!preg_match('~'.number_type().'|date|time|year~',$j["type"]);}function
host_port($N){return(preg_match('~^(:([^:].*)|(\[(.+)]|(([^:]+://)?[^:]+))(:(\d+))?)$~',$N,$y)?[(isset($y[4])?$y[4]:"").(isset($y[5])?$y[5]:""),$y[2].(isset($y[8])?$y[8]:"")]:[$N,'']);}function
count_rows($Q,$Z,$wf,$xe){$H=" FROM ".table($Q).($Z?" WHERE ".implode(" AND ",$Z):"");return($wf&&(DIALECT=="sql"||count($xe)==1)?"SELECT COUNT(DISTINCT ".implode(", ",$xe).")$H":"SELECT COUNT(*)".($wf?" FROM (SELECT 1$H GROUP BY ".implode(", ",$xe).") x":$H));}function
slow_query($H){$g=Admin::get()->getDatabase();$wl=Admin::get()->getQueryTimeout();$sk=Driver::get()->slowQuery($H,$wl);$d=null;if(!$sk&&support("kill")){$d=connect();if($d&&($g==""||$d->selectDatabase($g))){$Lf=$d->getValue(connection_id());echo'<script',nonce(),'>
	const timeout = setTimeout(() => {
		ajax(\'',js_escape(ME),'script=kill\', function() {
		}, \'kill=',$Lf,'&token=',get_token(),'\');
	}, ',1000*$wl,');
</script>
';}}ob_flush();flush();$J=@get_key_vals(($sk?:$H),$d,false);if($d){echo
script("clearTimeout(timeout);");ob_flush();flush();}return$J;}function
get_token(){$cj=rand(1,1e6);return($cj^$_SESSION["token"]).":$cj";}function
verify_token(){list($Bl,$cj)=explode(":",$_POST["token"]);return($cj^$_SESSION["token"])==$Bl&&in_array($_SERVER["HTTP_SEC_FETCH_SITE"],["","same-origin"]);}function
script($uk,$El="\n"){return"<script".nonce().">$uk</script>$El";}function
script_src($bm,$wc=false){return"<script src='".h($bm)."'".nonce().($wc?" defer":"")."></script>\n";}function
nonce(){return' nonce="'.get_nonce().'"';}function
input_hidden($A,$Y=""){return"<input type='hidden' name='".h($A)."' value='".h($Y)."'>";}function
input_token(){return
input_hidden("token",get_token());}function
target_blank(){return' target="_blank" rel="noreferrer noopener"';}function
h($Ek){if($Ek===null||$Ek==="")return"";return
str_replace(["&","<","\"","'","\0"],["&amp;","&lt;","&quot;","&#039;","&#0;"],$Ek);}function
truncate_utf8($Ek,$u=80){if($Ek=="")return"";if(!preg_match("(^(".repeat_pattern("[\t\r\n -\x{10FFFF}]",$u).")($)?)u",$Ek,$y))preg_match("(^(".repeat_pattern("[\t\r\n -~]",$u).")($)?)",$Ek,$y);return
h($y[1]).(isset($y[2])?"":"<i>…</i>");}function
icon_solo($q){return
icon($q,"solo");}function
icon_chevron_down(){return
icon("chevron-down","chevron");}function
icon_chevron_right(){return
icon("chevron-down","chevron-right");}function
icon($q,$yb=null){$q=h($q);return"<svg class='icon ic-$q $yb'><use href='".link_files("icons.svg",[])."#$q'/></svg>";}function
checkbox($A,$Y,$tb,$Pf="",$Dh="",$yb="",$Rf=""){$J="<input type='checkbox' name='$A' value='".h($Y)."'".($tb?" checked":"").($Rf?" aria-labelledby='$Rf'":"").">".($Dh?script("qsl('input').onclick = function () { $Dh };",""):"");return($Pf!=""||$yb?"<label".($yb?" class='$yb'":"").">$J".h($Pf)."</label>":$J);}function
optionlist($C,$Uj=null,$gm=false){$J="";foreach($C
as$Ef=>$W){$Nh=[$Ef=>$W];if(is_array($W)){$J
.='<optgroup label="'.h($Ef).'">';$Nh=$W;}foreach($Nh
as$t=>$X)$J
.='<option'.($gm||is_string($t)?' value="'.h($t).'"':'').($Uj!==null&&($gm||is_string($t)?(string)$t:$X)===$Uj?' selected':'').'>'.h($X);if(is_array($W))$J
.='</optgroup>';}return$J;}function
html_select($A,$C,$Y="",$Ch="",$Rf="",$gm=false){static$Pf=0;$Qf="";if(!$Rf&&substr(isset($C[""])?$C[""]:"",0,1)=="("){$Pf++;$Rf="label-$Pf";$Qf="<option value='' id='$Rf'>".h($C[""]);unset($C[""]);}return"<select name='".h($A)."'".($Rf?" aria-labelledby='$Rf'":"").">".$Qf.optionlist($C,$Y,$gm)."</select>".($Ch?script("qsl('select').onchange = function () { $Ch };",""):"");}function
html_radios($A,$C,$Y=""){$I="<span class='labels'>";foreach($C
as$t=>$X)$I
.="<label><input type='radio' name='".h($A)."' value='".h($t)."'".($t==$Y?" checked":"").">".h($X)."</label>";$I
.="</span>";return$I;}function
confirm($_="",$Wj="qsl('input')"){return
script("$Wj.onclick = () => confirm('".js_escape($_?:lang(108))."');","");}function
print_fieldset_start($q,$ag,$Te,$vm=false,$tk=false){echo"<fieldset id='fieldset-$q' class='closable ".(!$vm?" closed":"")."'>","<legend><a href='#'>$ag</a></legend>",icon($Te,"fieldset-icon jsonly"),"<div class='fieldset-content".($tk?" sortable":"")."'>";}function
print_fieldset_end($q,$tk=false){echo"</div>",script("initFieldset('$q');","");if($tk)echo
script("initSortable('#fieldset-$q .fieldset-content');","");echo"</fieldset>\n";}function
bold($cb,$yb=""){return($cb?" class='$yb active'":($yb?" class='$yb'":""));}function
js_escape($Ek){return
str_replace("<","\\x3C",addcslashes($Ek,"\r\n'\\"));}function
js_escape_key($Ek){return'"'.str_replace("<","\\x3C",addcslashes($Ek,"\r\n\t\"\\")).'"';}function
js_escape_re($Ek){return
addcslashes(preg_quote($Ek,"/"),"\r\n");}function
pagination($E,$hc){return"<li>".($E==$hc?"<strong>".($E+1)."</strong>":'<a href="'.h(remove_from_uri("page").($E?"&page=$E".($_GET["next"]?"&next=".urlencode($_GET["next"]):""):"")).'">'.($E+1)."</a>")."</li>";}function
print_hidden_fields(array$Ti,array$Xe=[],$Ki=""){$I=false;foreach($Ti
as$t=>$X){if(!in_array($t,$Xe)){if(is_array($X))print_hidden_fields($X,[],$t);else{$I=true;echo
input_hidden($Ki?$Ki."[$t]":$t,$X);}}}return$I;}function
hidden_fields_get(){if(sid())echo
input_hidden(session_name(),session_id());if(SERVER!==null)echo
input_hidden(DRIVER,SERVER);echo
input_hidden("username",$_GET["username"]);}function
enum_input($Ma,array$j,$Y,$hd=null,$sb=false){preg_match_all("~'((?:[^']|'')*)'~",$j["length"],$z);$nm=$z[1];$ul=Admin::get()->getSettings()->getEnumAsSelectThreshold();$M=!$sb&&$ul!==null&&count($nm)>$ul;$U=$sb?"checkbox":"radio";$xa=$M?"selected":"checked";$I=$M?"<select $Ma>":"<span class='labels'>";if($M&&$j["null"]&&$hd!==""){$tb=$Y===null?$xa:"";$I
.="<option value='__adminneo_empty__' disabled $tb></option>";}if($hd!==null){$tb=(is_array($Y)?in_array($hd,$Y):$Y===$hd)?$xa:"";if($M)$I
.="<option value='$hd' $tb>".lang(109)."</option>";else$I
.="<label><input type='$U' $Ma value='$hd' $tb><i>".lang(109)."</i></label>";}foreach($nm
as$X){if($hd===""&&$X==="")continue;$X=stripcslashes(str_replace("''","'",$X));$tb=is_array($Y)?in_array($X,$Y):$Y===$X;$tb=$tb?$xa:"";$ie=$X===""?("<i>".lang(109)."</i>"):h(Admin::get()->formatFieldValue($X,$j));if($M)$I
.="<option value='".h($X)."' $tb>$ie</option>";else$I
.=" <label><input type='$U' $Ma value='".h($X)."' $tb>$ie</label>";}$I
.=$M?"</select>":"</span>";return$I;}function
input($j,$Y,$o,$Qa=false){$A=h(bracket_escape($j["field"]));$Pl=Driver::get()->getTypes();$xf=isset($j["full_type"])&&Admin::get()->detectJson($j["full_type"],$Y,true);$rj=(DIALECT=="mssql"&&$j["auto_increment"]&&!$_POST["clone"]);if($rj&&!$_POST["save"])$o=null;if(in_array($j["type"],Driver::get()->getUserTypes())){$od=type_values($Pl[$j["type"]]);if($od){$j["type"]="enum";$j["length"]=$od;}}$Ma=" name='fields[$A]' ".($Qa?" autofocus":"");$pe=(isset($_GET["select"])||$rj?["orig"=>lang(110)]:[])+Admin::get()->getFieldFunctions($j);$Fe=(in_array($o,$pe)||isset($pe[$o]));echo"<td class='function'>",Driver::get()->getUnconvertFunction($j)." ";if(count($pe)>1){$Uj=$o===null||$Fe?$o:"";echo"<select name='function[$A]'>".optionlist($pe,$Uj)."</select>",help_script_command("value.replace(/^SQL\$/, '')",true),script("qsl('select').onchange = functionChange;","");}else
echo
h(reset($pe));echo"</td><td>";$if=Admin::get()->getFieldInput(isset($_GET["edit"])?$_GET["edit"]:null,$j,$Ma,$Y,$o);if($if!="")echo$if;elseif(preg_match('~bool~',$j["type"]))echo"<input type='hidden'$Ma value='0'>"."<input type='checkbox'".(preg_match('~^(1|t|true|y|yes|on)$~i',$Y)?" checked='checked'":"")."$Ma value='1'>";elseif($j["type"]=="enum")echo
enum_input($Ma,$j,$Y);elseif($j["type"]=="set"){preg_match_all("~'((?:[^']|'')*)'~",$j["length"],$z);echo"<span class='labels'>";foreach($z[1]as$X){$X=stripcslashes(str_replace("''","'",$X));$tb=$Y!==null&&in_array($X,explode(",",$Y),true);$tb=$tb?"checked":"";$ie=$X===""?("<i>".lang(109)."</i>"):h(Admin::get()->formatFieldValue($X,$j));echo" <label><input type='checkbox' name='fields[$A][]' value='".h($X)."' $tb>$ie</label>";}echo"</span>";}elseif(is_blob($j)&&ini_bool("file_uploads"))echo"<input type='file' name='fields-$A'>";elseif($xf)echo"<textarea $Ma cols='50' rows='12' class='jush-json'>".h($Y).'</textarea>';elseif(($pl=preg_match('~text|lob|memo|json~i',$j["type"]))||preg_match("~\n~",$Y)){if($pl&&DIALECT!="sqlite")$Ma
.=" cols='50' rows='12'";else{$L=min(12,substr_count($Y,"\n")+1);$Ma
.=" cols='30' rows='$L'";}echo"<textarea $Ma>".h($Y).'</textarea>';}else{$Cg=!preg_match('~int~',$j["type"])&&preg_match('~^(\d+)(,(\d+))?$~',$j["length"],$y)?((preg_match("~binary~",$j["type"])?2:1)*$y[1]+($y[3]?1:0)+($y[2]&&!$j["unsigned"]?1:0)):($Pl&&$Pl[$j["type"]]?$Pl[$j["type"]]+($j["unsigned"]?0:1):0);if(DIALECT=='sql'&&Connection::get()->isMinVersion("5.6")&&preg_match('~time~',$j["type"]))$Cg+=7;echo"<input class='input'".((!$Fe||$o==="")&&preg_match('~(?<!o)int(?!er)~',$j["type"])&&!preg_match('~\[\]~',$j["full_type"])?" type='number'":"").($o!="now"?" value='".h($Y)."'":" data-last-value='".h($Y)."'").($Cg?" data-maxlength='$Cg'":"").(preg_match('~char|binary~',$j["type"])&&$Cg>20?" size='44'":"")."$Ma>";}$Ke=Admin::get()->getFieldInputHint($_GET["edit"],$j,$Y);if($Ke!="")echo" <span class='input-hint'>$Ke</span>";if(count($pe)>1)echo
script("qs('select', qsl('td').previousSibling).onchange(null, true);","");$Xd=0;foreach($pe
as$t=>$X){if($t===""||!$X)break;$Xd++;}if(count($pe)>1)echo
script("qsl('td').oninput = partial(skipOriginal, $Xd);");}function
process_input($j){$We=bracket_escape($j["field"]);$o=isset($_POST["function"][$We])?$_POST["function"][$We]:"";if($o=="orig")return(preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?idf_escape($j["field"]):false);if($o=="NULL")return
Driver::get()->getNull();if(is_blob($j)&&ini_bool("file_uploads")){$l=get_file("fields-$We");if(!is_string($l))return
false;return
Driver::get()->quoteBinary($l);}$Y=isset($_POST["fields"][$We])?$_POST["fields"][$We]:(isset($_FILES["fields"]["name"][$We])?$_FILES["fields"]["name"][$We]:null);if($Y===null)return
false;if($j["auto_increment"]&&$Y=="")return
null;if($j["type"]=="set")$Y=implode(",",(array)$Y);if($o=="json"){$Y=json_decode($Y,true);if(!is_array($Y))return
false;return$Y;}return
Admin::get()->processFieldInput($j,$Y,$o);}function
search_tables(){$_GET["where"][0]["val"]=$_POST["query"];$wj=$qd=[];foreach(table_status("",true)as$Q=>$R){$Zk=Admin::get()->getTableName($R);if(!isset($R["Engine"])||$Zk==""||($_POST["tables"]&&!in_array($Q,$_POST["tables"])))continue;$I=Connection::get()->query("SELECT".limit("1 FROM ".table($Q)," WHERE ".implode(" AND ",Admin::get()->processSelectionSearch(fields($Q),[])),1));if($I&&!$I->fetchRow())continue;$w=h(ME."select=".urlencode($Q)."&where[0][op]=".urlencode($_GET["where"][0]["op"])."&where[0][val]=".urlencode($_GET["where"][0]["val"]));if($I)$wj[]="<li><a href='$w'>".icon("search")."$Zk</a></li>";else$qd[]="<div class='error'><a href='$w'>$Zk</a>: ".error()."</div>";}if($wj)echo"<ul class='links'>\n",implode("\n",$wj),"</ul>\n";if($qd)echo
implode("\n",$qd),"\n";if(!$wj&&!$qd)echo"<p class='message'>".lang(78)."</p>\n";}function
help_script($pl,$pk=false){return
script("initHelpFor(qsl('select, input'), '".h($pl)."', $pk);","");}function
help_script_command($Ib,$pk=false){return
script("initHelpFor(qsl('select, input'), (value) => { return $Ib; }, $pk);","");}function
edit_form($Q,$k,$K,$Zl){$Zk=Admin::get()->getTableName(table_status1($Q,true));$T=$Zl?lang(38):lang(111);page_header("$T: $Zk",["select"=>[$Q,$Zk],$T]);if($K===false){echo"<p class='error'>".lang(89)."\n";return;}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";$dd=false;if(!$k)echo"<p class='error'>".lang(112)."\n";else{echo"<table class='box'>".script("qsl('table').onkeydown = onEditingKeydown;");$Qa=!$_POST;foreach($k
as$A=>$j){echo"<tr><th>".Admin::get()->getFieldName($j);$t=bracket_escape($A);$h=isset($_GET["preset"][$t])?$_GET["preset"][$t]:null;if($h===null){$h=$j["default"];if($j["type"]=="bit"&&preg_match("~^b'([01]*)'\$~",$h,$oj))$h=$oj[1];if(DIALECT=="sql"&&preg_match('~binary~',$j["type"]))$h=bin2hex($h);}$Y=($K!==null?($K[$A]!=""&&DIALECT=="sql"&&preg_match("~enum|set~",$j["type"])&&is_array($K[$A])?implode(",",$K[$A]):(is_bool($K[$A])?+$K[$A]:$K[$A])):(!$Zl&&$j["auto_increment"]?"":(isset($_GET["select"])?false:$h)));if(!$_POST["save"]&&is_string($Y))$Y=Admin::get()->formatFieldValue($Y,$j);if(($Zl&&!isset($j["privileges"]["update"]))||$j["generated"]){echo"<td class='function'></td><td>";if($Zl||!$j["generated"])echo
select_value($Y,'',$j,null);else
echo"<code class='jush-".DIALECT."'>",h($Y),"</code>";echo"</td>";}else{$dd=true;$o=($_POST["save"]?isset($_POST["function"][$t])?$_POST["function"][$t]:"":($Zl&&preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?"now":($Y===false?null:($Y!==null?'':'NULL'))));if(!$_POST&&!$Zl&&$Y==$j["default"]&&preg_match('~^[\w.]+\(~',$Y))$o="SQL";if(preg_match("~time~",$j["type"])&&preg_match('~^CURRENT_TIMESTAMP~i',$Y)){$Y="";$o="now";}if($j["type"]=="uuid"&&$Y=="uuid()"){$Y="";$o="uuid";}if($Qa!==false)$Qa=($j["auto_increment"]||$o=="now"||$o=="uuid"?null:true);input($j,$Y,$o,(bool)$Qa);if($Qa)$Qa=false;}echo"\n";}if(!support("table")&&!fields($Q))echo"<tr>"."<th><input class='input' name='field_keys[]'>".script("qsl('input').oninput = fieldChange;","")."<td class='function'>".html_select("field_funs[]",Admin::get()->getFieldFunctions(["null"=>isset($_GET["select"])]))."<td><input class='input' name='field_vals[]'>"."\n";echo"</table>\n",script("initToggles(gid('form'));");}echo"<p>";if($dd){echo"<input type='submit' class='button default' value='".lang(113)."'>\n";if(!isset($_GET["select"]))echo"<input type='submit' class='button' name='insert' value='".($Zl?lang(114):lang(115))."' title='Ctrl+Shift+Enter'>\n",($Zl?script("qsl('input').onclick = function () { return !ajaxForm(this.form, '".js_escape(lang(116))."…', this); };"):"");}echo($Zl?"<input type='submit' class='button' name='delete' value='".lang(117)."'>".confirm()."\n":"");if(isset($_GET["select"]))print_hidden_fields(["check"=>(array)$_POST["check"],"clone"=>$_POST["clone"],"all"=>$_POST["all"]]);echo
input_hidden("referer",isset($_POST["referer"])?$_POST["referer"]:$_SERVER["HTTP_REFERER"]),input_hidden("save","1"),input_token(),"</form>\n";}function
file_upload_form_script($fe,$jf){$vg=ini_get("max_file_uploads");$_g=ini_get("upload_max_filesize");$Ag=ini_bytes("upload_max_filesize");return
script("initFilesUploadForm('".js_escape($fe)."', '".js_escape($jf)."', "."$vg, '".js_escape(lang(118,$vg,"'max_file_uploads'"))."', "."$Ag, '".js_escape(lang(119,$_g,"'upload_max_filesize'"))."')");}function
compress_alphabet(){return
strtr(implode(range('"','~')),"'\\","!\n");}function
decompress_string($Ek){$Fa=array_flip(str_split(compress_alphabet()));$u=strlen($Ek);$mm=($u?13*($u-1)/2-$Fa[$Ek[0]]:0);$Ya="";$uj=0;$vj=0;for($p=1;$p<$u;$p+=2){$uj=($uj<<13)+$Fa[$Ek[$p]]*93+$Fa[$Ek[$p+1]];$vj+=13;while($vj>=8&&$mm>=8){$vj-=8;$mm-=8;$Ya
.=chr($uj>>$vj);$uj&=(1<<$vj)-1;}}if($Ya=="")return"";return
function_exists('gzinflate')?gzinflate($Ya):inflate($Ya);}function
inflate($Ya){$bg=[3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258];$cg=[0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0];$Jc=[1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577];$Lc=[0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13];$J="";$G=0;do{$Vd=inflate_bits($Ya,$G,1);$U=inflate_bits($Ya,$G,2);if(!$U){$G=($G+7)&~7;$u=inflate_bits($Ya,$G,16);$G+=16;$J
.=substr($Ya,$G>>3,$u);$G+=$u<<3;}else{if($U==1){$ng=array_merge(array_fill(0,144,8),array_fill(0,112,9),array_fill(0,24,7),array_fill(0,8,8));$Mc=array_fill(0,30,5);}else{$mg=inflate_bits($Ya,$G,5)+257;$Kc=inflate_bits($Ya,$G,5)+1;$D=[16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15];$Lg=array_fill(0,19,0);$Kg=inflate_bits($Ya,$G,4)+4;for($p=0;$p<$Kg;$p++)$Lg[$D[$p]]=inflate_bits($Ya,$G,3);$Mg=inflate_table($Lg);$dg=[];while(count($dg)<$mg+$Kc){$Pk=inflate_symbol($Ya,$G,$Mg);if($Pk==16)$dg=array_merge($dg,array_fill(0,inflate_bits($Ya,$G,2)+3,end($dg)));elseif($Pk==17)$dg=array_merge($dg,array_fill(0,inflate_bits($Ya,$G,3)+3,0));elseif($Pk==18)$dg=array_merge($dg,array_fill(0,inflate_bits($Ya,$G,7)+11,0));else$dg[]=$Pk;}$ng=array_slice($dg,0,$mg);$Mc=array_slice($dg,$mg);}$og=inflate_table($ng);$Oc=inflate_table($Mc);while(($Pk=inflate_symbol($Ya,$G,$og))!=256){if($Pk<256)$J
.=chr($Pk);else{$u=$bg[$Pk-257]+inflate_bits($Ya,$G,$cg[$Pk-257]);$Nc=inflate_symbol($Ya,$G,$Oc);$sh=strlen($J)-$Jc[$Nc]-inflate_bits($Ya,$G,$Lc[$Nc]);for($p=0;$p<$u;$p++)$J
.=$J[$sh+$p];}}}}while(!$Vd);return$J;}function
inflate_bits($Ya,&$G,$bc){$J=0;for($p=0;$p<$bc;$p++){$J+=((ord($Ya[$G>>3])>>($G&7))&1)<<$p;$G++;}return$J;}function
inflate_table(array$dg){$Q=[];$zb=0;for($Za=1;$Za<=max($dg);$Za++){foreach($dg
as$Pk=>$u){if($u==$Za){$Q[$Za][$zb]=$Pk;$zb++;}}$zb<<=1;}return$Q;}function
inflate_symbol($Ya,&$G,array$Q){$zb=0;$Za=0;do{$zb=($zb<<1)+inflate_bits($Ya,$G,1);$Za++;}while(!isset($Q[$Za][$zb]));return$Q[$Za][$zb];}if(isset($_GET["file"]))load_compiled_file($_GET["file"]);function
load_compiled_file($m){if($m==""){http_response_code(404);exit;}if($_SERVER["HTTP_IF_MODIFIED_SINCE"]){http_response_code(304);exit;}header("Expires: ".gmdate("D, d M Y H:i:s",time()+365*24*60*60)." GMT");header("Last-Modified: ".gmdate("D, d M Y H:i:s")." GMT");header("Cache-Control: immutable");ini_set("zlib.output_compression","1");$Cd=pathinfo($m,PATHINFO_EXTENSION);switch($Cd){case"css":header("Content-Type: text/css; charset=utf-8");break;case"js":header("Content-Type: text/javascript; charset=utf-8");break;case"ico":header("Content-Type: image/x-icon");break;case"png":header("Content-Type: image/png");break;case"svg":header("Content-Type: image/svg+xml");break;}switch($m){case'favicon-blue-0f5ce53a66b1e25395d0048da369f19e__aff407a3.ico':$e='AAABAAEAICAAAAEAIAC6AQAAFgAAAIlQTkcNChoKAAAADUlIRFIAAAAgAAAAIAgGAAAAc3p69AAAAYFJREFUeNrV1wEEGmEYh/FztCYBRATANhCAAEGAEGZowEUFhM2G6A4QAJksoMi2AYRlAxgcAUgthAS2yTFo5d2DDzbO6r2PhB9APY73z+cUn3+6qbsJcFGCjxlCbPHL2CLEDD5KcG0EPESAH5ArfUeAtDbgCb5BElrjsSbgI8SSD5qAM8SSsyZAbNIErCGWrDQBTYglTe0ZNnCAKB3gJR2iAnwsIBdawEchyRC9jompoYUe3hg9tFCL+dNX2ivo4wEcpTT6EF0AsEMHeTgXyqODnf4M489phC7aeGq00cUIK1s7sLr1DryEWPJCE5DBBJLQBJkkO9DAHnKlPbwkO/AMjuGijCGWiCD/iLDEEGW4f/2WIuA3qnBiZPHIyMKJUcVJe4ZHDJCDc6UcBjhqz/AEMSKMUf9PTA51jBFBAN0X+AKJEWGDr8YGESTGZ02AB7HE0wSk8B6S0DuktDvgYgRRegvXxsuogjnkQnNUrL8NUUSAKUL8NEJMEaB4x4/TG/gDMBOIUjRp9w0AAAAASUVORK5CYII=';break;case'favicon-blue-17e440832c1eac07527560a0d6f0d2ee__aff407a3.svg':$e='+<bATb3V?$so%el,wEIwK&mlYjGZ$a8-HGs8y$j)-UBmQx`Cf?>]C6?xmhS1<w
ZNSJl63"aZ]nB<rgk|tG)vC,pHYx;Wb2NVXBMxd!lQ=g0"!mM[]c*SW?7
E^]B7t[fHolNczfsymCW
CM~8Ult[(brlOx`71nDc
H;N~ybgNd*l6LMbRk;>%06rQeBnDc14r_7Z0b9uqO=xV5=22d05hr#V]F`V>ZD,#JA9[$XEV-*TBfz7Z%
rEJw!G!cT+[7>-u:iVU)N$iv%ySN.`._&}sV@FUIuH)%cY-0#OXWPT8t./3Oid8~R]3?9n;/Qs
Y2!K`1Q9d
tys6C=xmJfXFCT%0xl`H&%njK7`N[.q6jET6kV
VqmiJoIrZ#rsP~q0vDN<FH1w9l4R-?A#H:#onn0@0]3dNk3,E{<7r;2q
u)F!d(nhskXC~JT4N!~!g52r#`3hF[%j
oPE~ZV(W_~g#t?WXQCxEe7)ZQKGxei4.gu_R>pAL]HDZf!uSL)$_)^vZ:Xk![_HKh=C|S~PjqHcgjUutq#-~?PmA#<MYyg2R';break;case'apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__aff407a3.png':$e='iVBORw0KGgoAAAANSUhEUgAAALQAAAC0CAIAAACyr5FlAAAK7UlEQVR42uzSgQAAAAACoP2ln2CDYig9QA7kQA7kQA7kQA7kQA7kQA6QAzmQAzmQAzmQAzmQAzlADuRADuRADuRADuRADuQAOZADOZADOcbeOUa502RxeG0bX17btm3b/tu2bdu2PbZt21bY+6xqPduTSSddSd1zPyTdVZk+p5+prr51f7d83ixVOXXH55Yte6No2r25Qy/O/Pg7OB/4ykFO0cC/4FBma6qq2Tsmb+SVGe9/7f86zWhMFx+HQ5mjs7XmwITMT77LXe+R04WOdFdw+Ka1xOzL7vML7rTLTnd+RMHhW+Z01Owfz911izOE8IMKDl8wh6W9dPHLbsFCOD/Izyo4pB8z3EyG4GPJK5rTqeCQ2HgEGECGeL5MVHBIPAM1CAvh/AkFh5Rvrf/13STr0x/UHpmO4yXzn+7+3pev+VA0puN/fX/hDyk4fOSBkjPg96KN09KRN/KqbuBoSzsnGtNRz8NFwSFBDJRAeDdwCOsqS8/86Nu9gYP4mK25WsGBaXVNlpS8plORFeuP5k/ZkPbVnLj3J0e9Nib8uWEhj/QPvPvz8zd/eAa/54vzfOUgp2hAs6kb0uhCR7rXN1s0I42AN7dNDxxYY/CG3sCB1+wb66dwNLVaQ5Jqlu/L/XxWLPf+hvdOu8X5qS9mxS7fnxuaVNvcZtXcaiyI6IeDN9KyFW/3Bg6eTf4FR1p+0/ydWS+ODL3xfe6lsc6feGlk6IKdWWkFzW5Za+WG6YNDzF5bWIx1GQ7cUpXr+3DklLQs3pP9zNBg7plX/NmhIUv25OSWuv4KwFK7TjhsjZXic2dRQsYH3/h3OFLP6oSDP+rLcDAV6DM3jttjEu83P961gYRUDJ1wVGz8wtZYIb7Wn136b41bk0/phKNs+Zu+CUd6QbPAwmzeb158ZlHPECFVRy8cGz6nsdNuE5OP0kUv/gscCUd0wlE07T5fg4PFgQ3HCni5MCEWwrm8zScKNd3G7EEnHDX7xnGkes9occTe3pgz8I//HgDVAUfu0Et8Cg5eQxi6xT0wuQ9amNDSbtN0GBEOnXDUn1nytxEi6aQ42JEbIRo3hW/XCQfRDt+Bw2JzvDk+QgoshL89MdJqc7gRjqawrRzBs774ibWuWByvOzabg3hj4Fp/hGPaxnSJsBA+a2uGGx8rLXGHxPHCSXc47Vax0F8853EO1p2c73ePFeJO0mEhPDKtzl0T0raUM+I4XrVjiDhlb67J7vdrQp9+NyGdKuGwIXzWlgx3vcoS4/q3s4wl4mx7RkDlxi/97lWWuKe8cLw6JtzFIJgOOLI++6G1pkA06CyI9bsgmKRYiDdbF8PnOuDAC8bf7LRZRBuZwucKjps+OKNn4c1lOPDKLf1EG19beFNwsGTfGzjw5ug9oplPLdkrOEj2IfDQGzgyP/2+EMr6UrKPgkOkCboOB54/5jqntVP+NEEFh0sJxuQPc6QbL5n/jN8lGN/+8Vl54bjrs3NKmmCgfTg1Wl44Pp0Zo0RNBtri3dnywkE2q5JDGmikz4gcDrn81o/OksjodSE1ZPiykHrt4XwZ4dh4vMA1OTXPF3c+TcSY4ZNwOJzOT6bHyEUG2ginUxVv8Yi1d9oHL06UhYzhS5M6uuzuKPs00dWyTxP9q+wT/4hrDuUhGzF1YOP9M0jl3CuWJODdvUpWOM1oTAzUTwvGRafXvzHOpCmDb02IiM2oN6zUZC5L7aRikKpDKhfDA84HvnKQUzTQVKlJhpDTkZXPDQ8xDxYvjAg9G13ldKoiteYwu915LKyceR8juRcfImhoj4dXOBym5EKVmqxu6Nx0vACBvCexeH1c+JYThTUNXZoyKVT25bUdjCWUWkDobIh+elQYpRmOh5VX1HZoyswAxwdTo4orexzga2yxBCXU8M89fVP6l3Niqb1xS0/CrDSmC4U66M6PBCfUNLZatR5aQXnbR9OiFRzGLtkz9+x9VQymBRV1ncSzk3MbUQwExFefCK/YF1C6P6CUD3yNSqvjFA1o1vs5BHQ+PSRYLNkbZQoO/LOZMUJAZn5Dovfx9GiRz2GsKTjwdydFVtV3aqY3Bp63J0RwwQoOj2aCPdQ3ICK1TjOxhSXXPtAngEtVcHgnTZBFuIiUOhNiwfRTXKSCwxPWTaz6XEyVw9tRSS7gTFSliOgrOEyUYIxY8nBImc3uBUT4oweDSp8fLmIqCg5TZp8/3C9gxPKk3eeKqd1m6FDCj/Ouu+tc8fBlSUyAxAUoOOSQJjzw1QUK62w9WUgFN8IVblnESc1ropjTwIUJ9391wQBpgjdM6Vbu/uw8AZIxK5PnbMskF4ShheVcgl1ZRS2VdZ2dlr/l4/CBrySrEhyjAc1oPHtrJh3pjrzAAN2K6U2Jmm7/5JyhuhgFh7HG/ZNXmsDQpeAw0N6bHCUvHGrhzVibuTlDXjjmbs9UcBho7GshLxyE6RQcxoYgqekpIxls3UJcRMFhuCKSab901cCEFlLBYawt25cjFxwrD+SqNEHPGYp1WchYdTBP5ZB62rafLjK/4o0Aq+YVU9nnF+KqHxsQZE4yHh8YRFazhik4vCiqZkc3UxXtIFt90a5sIZ5WcHjZ8spa2bLJ608ZLqDvvLj8slbNbKZETUha5m3PvO/LC57HguV7BrDiqnZNmbfg0JNR3GWxkw+Gbva2jwwvPcgq7pezY4+ElHdZHfovXsFhiDH/Z2s3nY2hhNxj/qHdK53l2fH62PCFu7K52TChv1oVsCo4DLS/ZkUkZjdqPbSGFktsZv2+CyWsfjEtQH+mU49PM/aqpQsPLCRx/AjyNa2HRi8uG9eMNpXs88SgoNqmrt6nBCN5RW9NYDspt5ExhoUxnA985SCnaND7dGXU948OCPRcso/KBHtqcHBhRZtmessvb3tyUJDKBPOE/dsLgni+mNPisxrEq5OCw3D7z9VOahoz8dRMZmQpM2Pl8rycQ6oSjJkwMlEwlRaSp55KMPa0dZ+hyVqG0+nN0nUU9vhgSpTKPjepNIEiT7xwincZz1htY9feCyUUEVTSBDl0K+zTSfhLxKncbkx0wlNq5+3IemV0mCl0KwoOBGcuSJWoA0YtL7ck6mWXtKCFpKSkC/In3lw005rSrTzYJ+DN8RGUfhu7KoVxhZ0MWIUJSapJy2+iKCBjDOMBH/gaklhzKLiMBjSjMV0orEB38+pWFByUjpRXmjBjc4aCw0Cj3oG8cDBTVnAYaCx6oTiVkYx7vjjf1GpVcBhrpPnLCAdL9irZx3Br67SJEn2yOMWGPJRSqtIEqb4lFxykiqk0Qc8Zy1qykLF0b44mv0m2jdeoFcnmJ2Pc6hTN86ayzyl/TnKvmclALUEimYLDaxs0kdppTjJ48Nm9TobSrZD4effn500V0kCnqQlTcHhdzmSSbSLZI6Gk2jTSJgWHKPpzNLT8SZGC5XFH7sDufyLbSMFhxp1vtp4qonCxJ7EgKEc9CDn2B1IbALa220jgQJdmNBY8y2CRoK0miyk4hFEbn2oIbq/hgapq8Z7sPKnV9AoOMR1BkMjSF9XsH+kX6BoQj/QPpMb+uiP5qFF0TCwUHHIade/ZTnzJnpyJ61IHL0pgdz7yQFEskvlHTiEf+MpBTtGA4DevylLsJ/endulABgAAAGCQv/U9vmJIDuRADuRADpADOZADOZADOZADOZDjBTmQAzmQAzmQAzmQAzmQA+RADuRADuRADuRADuRADpADOZADOZADOZADOZADOSAe34f5izVe/wAAAABJRU5ErkJggg==';break;case'logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg':$e='(]^+JbP.FqjXYdorFxH%oTmn1#,Na[(-^<}T{`+Ahl-RItQoM;{4bK}l["$V3F6U&V6Ey@S8#w=t>3kaN[hLow+fWEUH+K<LoXqyEy6JupFy-JyK4S8q(7tl96;KLl/F|,Cz)p?p(B)[axu/4u77-)nvU
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
n9lrNOU01c?:p/5y16+Zkgo}`M)D6xm>7RiQ_b%p%ocllip!0).myrT"w[53iGRZBt8z<.d6p9("_WYy1v;v.xBx%3c3&hfawJgMtuxeflyK1.-:wQo"f_z&)W';break;case'jush-b3a93b18444da26820ff61746521dede__72e4fe51.css':$e='+UEmPb3V?!K0u25Dm[994[Zg@N#Q)YOC=2R_hE~4)=>cbdia55M)rQq_opI7=E.gy$2_wn3[@yoG6r~P5/:mrvY<e>#2+8qezLLv^&nr;/Kkr(>?R(rf#PZ<Kx
br^LS(>/*E-?WzeLSW_J
;*l
(asND-)j;m4/f-BIQ%S$]jg`lK"7X[Woi<6n<ErGn[ASke
cM6fo
Ky:?d|y4Z`/MKF8_iz@9f#<b1@MaLgh0efOIpYz&+xn<6xNY
d<~>ajCRq4s@jh
caxtV~2DNi9ioWoHqA9#OBh[!x5h*jN+q=`bmxSYd@yVW[J$)|db!4XItVe2/XU=San(wzD4EHl0a(LT*+#/I{HkOQ@+p7OU>7LCKJ)XgXJ0ht%5=cOF]A#h3y>xj
CPbQe)?*P`i3V~D/=qVG-dKwTh&h0H`Bh6D#U{g+4O4p2=9CtsQ/6U+vL<<[BwoX?2A6[c[V]D4-0UY0<f68Rw&}-5Kr^"[Lrv)&Bo_Q>]coooyj>sL9EEvT;B"HxR.k8B
^Q5llt~q7xBV~/n!91bSK$-ui1OQU0Jb$`Vf`/xBJPix,!jg:C6a0@xf^+|r7RpN*I/2:M%^huBD0`%<qSsC;K:6QF=r``duu$_:GGnAJ+yY4!,e.H+17juw;`Qv?UzH/[xK7OTM3Z[qLq^Z^+TawmRd!sSIPOxE!SvhF<|rj:/l,BOJ
mSuF$F"Zd+H*kq9$y!*@F1uY
f-gLsy-15W-N0hLvuJRq9Wpsq]/I!y*0G?:_rlbTt6D;G*GS~^a@HY-C!&62>2?z$:C?FDZ<faV50J@TPaw$ho!P$-okZoO1r^E-sl/oF>NEK8#EKBBKVZ_<mqh4swu)jYp;|+Kvi!"N+01_T-&=mH9s.pB9Qu!OD3m.Qc<m(T*j6SBmlJy+%v{79w|"Bn(V=Rj"ND,Ek(pjZKL^zAr1(P>e8nI&&y=E]uD6uOTPjvG[(AR]Kbr`]M|A(;|wf`C%Khwq200kz[t6)?ZIb&Trvi7%2NO?:O%Ht2"ee:3Fvl!s
VMlfy|MRtX';break;case'jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css':$e=',Gjwm6?!R"-YJmoGR`r@~cEv;#i.*-_KUyr[0$CF,>/n=#+liP*01.(73:+G.C]Ek+^-h&|hnGDq1:ccpxU98SxFh5MU%c+]DCcezAcUOWmDiL$
)yZA,ICx<`.i#E%U;lo*kf6u&LQx+!%1t]iP#G9;zGT,4U2"ha>hB#am`y1YU6$z!l#C%';break;case'jush-615bc0b9720a1de8edd2c6876a3495b6__aab91337.js':$e='(hk]`!>p9CvwpHP(hq[*!NJF5FML(97K>/e-sd5Yd_qN;*HB8+(1wUgZ|7~w/mVWtDMgGM~htv^jmBm4eb;y*03o(V96q=%wGFJ+{#Pe*,NlHjy7BFQgK&`=9x
w}KS2<C8mz.p;8y@A]]KOHE=LG+ZJY69hi]OmLk*<i_y>A?DKpNY;Eh-vl?}:fK#yN/L7`JV"_gO#zW]A%bgx5C2iYwmPaMJuX*RsGB}TjvRN#`{,KUMi
bus4R/p(^I_~S@p#Wvq3(AP(py_~k|UY$"Y:=8^YN?&4x@x~MViEy:O(oAE*[
/b]C##(si>X2j<-;t|e++Ydp^P$&7@4#My:^SMObTpj$oO
]5_v>omWdd
W&>x5WND
<.qP/
.cxv8P@DKWL4pf5eG(E*ZwL>,cxPpMDW+CxX@`u:B*Sbo#T([)~.G
.
y2SX{A*Y8/8uHsuehCK>W7BWfyM&)($&l4;IZCY4v1M`"Syqu?KrH$x51f{:S`>/Rv9U"yTbR<3R@cHL3l!obgeXKa5w9ZBN#FH&_2S7~P=6MGsfaEaDHLW/5/:_*!#k8K2IDdF9e##HDk$e{V~qLqdZ}o<EXI=LR2Pw_J=322R&j!K+-+9XPj1BNq;y62xO8!~58w*]=,YI}<.A@!!AyD}IO0A"dE-2P-0)QMVuz,-v<1ZQ-5&?H6^.8HGF.0<@|nj=oK[^KwG44a-KS&{k4f+!&a>[Y"Z0w6/]
*}2]?%T>Z@,XS<8DjxbilrGIA{pIL.MHJOvZnzI0%~FT8cj/aO&5EL5TXi2_P4IkKS.ORpvNd(Ri
K?gF_.3sv8*r|_dTwC|+<OA6WqhD:k9.q6z)5r.dw:>9~-#
Jm<HsM>:]D4TBmv
[i><]8@
F_`]&TW?p2D^Ou[`EKqW0TsmX@*l)IBf$BOl5Y7,syLtMP+M[*LV1Zj<x%-]XyR2WoV;Q:1wmXRb<pgmQiJBW`VMah[pf[0K&K
Oo2EU^8EQUf.EyCw`-4f=.#JjC>b3dYB4[I:XlrI=Igp%PrS;c!/^B
T-Yn{N@`fjnHiT[)om&NC8`wywgf*bVLFha5}KXHE9]GBMYF)b2)A%|N{,Bi$y0M?q}I`6z,BUV6Gid6xn_SVV9$VeV;S+5mr+HGN]h`[CE$Za$fIe*SVR[c_;n]#x4.~kWawhx9:$O7TB?P=fA&>RhI*=0n6V8xoTUGN]+&ZN7Xy+49hi!X!nqq_F3XbxnTiX`A*O+)D>bjUXBi=yq`?q~
8a(q^^LQz]ho=jZigUF7benA#<>"H(7
yb/BVj>7$EfR!cYe3d2n5^{0b(,R|8bWdc6mX4-8-
+W&4PjB#0X{5E-yi?q%kNEv*76CTy6D%nfJE],&l$Vi9=aI?#Rsue;p,kiP6/St/cR`ga/*A3jCHNDrjiG7?McOVjkWqdDH[G+?wC,_8[/&3
Jd/G")SYCTnMI>&%-*6{Q:/swD9_mkD`K?FJ;}li?PH{G|?XL
"Y0IV2sn^uELH2i{!,BnQle`f2Zz-oN4A?3=ifbw]M^6ov410!^EaZ0Uc3N<_KjYe~kX(}KzUu",K=^gdP3F.OEFfFm#aiyZ5Q1%=-[c+^AtJ*E1MCIQdn53"w<G9w5w48CUhWR;f%axaV
K;=Tg0H6e]=aWBkUG*3m2t+tC3?TL8V;Z]UVS&C`lTH:)X<nBDy!jE<9L7!5wu0nyjyO=+4G>pSv(x
C!D),r0*o{wi$pEW39(!u(BgE(Aq2k24WL6GeZ!q6jTFltTCB

9Uqb,vj6!2Yc
28gE,nESwuO4-@_q%~krA4i)?=FralR"cl[96CCr%3yo+M_Kue:^u>7IfD"ClFD/8SGHV3P$"E#0L04"JqxS8@<Kc|t:p*fAV;!8:LXjJoQ{1[x3w>O;I0<JK"CBnReimy+!N]
_h]c-1*[(jee&Ouiy+NoKJ=n,&>Ii#9="+l<U#%MJ>V".qDGNw8iAy._@BA]>c42y$M@{H
>cS3=7H;%B2ixzHX"J+93g6r(A/BI#kBg_8(M`+N?g*L#o<#[q5=A@X])qmU*0$Q0}<Oe$N}u2Hg$mw
cc99O)1R"X?]/R4:mun&;*Gebs$j9=?E
d]Ta)INLAUmS77z)1j-(uPs0P"^>SQ;=Yy
bSoj2BVYla"VAMewH;4fLry#PY-^MEf?H*f+[xFp%CWUEZQD[|ZLnufDu(xeFzvkN.M0)Xetf8T0huA1?=xqxI@D61<JDMLurO!X(jW`Jw
@ZEwVD]dr_E/A-YNE!Re(W",[oy+2D,27YC_&5H4eokPfK>y+%%9G]}CRZaEak@glWE#Nh[:&k$L((Wy:KO<b
La=u3euZsjP:GgwajR*R&0gKiH"d3_SnW$jk7q9r{bjb"Xql>rOTFY>l4wDS:hI<|vBNRS
ks(LRG$)=qSMnUU^IzsE*meFc{TE4J.nx(/|N<?-^&wM;O2Tx-(5-4mk3bx.w>fyH<%>),3>pR,+f8&:GMySjb5>tl:5mz
Kp3oQ%|%|8@qqO:/<?G`&uqLjQ+5.F}7(A
&(P]AEGHoxRXX-uF
}4=h6,O;,&=0[w[wR!u&LSA]8oU>.N3Y/.$])`%.0TY73nNd57!nXCuS]+v[Rd|gqBu]WKte8p>kj%|D
yPNH4zU@4XjcoO?~@iPvfGqk$:t[UW:AHL9_dTrxFo1BGWgY:Q;GE8)-l}uv=LL0p9@iRgP(j+1c2[y6A(upby$B-Os}:P6!*~*N@^hmLaVm(RY~*H8MY:$sj%WgQipC
@N?FP
z$~@3.pb=Rhevrg@*Qek1FD_^sQ&R*d-I+T%Y?^"U%y->C]r-!QVM2|f6
>1rw!aR!.c8/;PQtA.&mpZ$6I`"*VG3J?NTr>#@a80RK(>,,|-aipDd88S-Qh
PU$uHvS&E,(8,e}+YU78`P|IJbiyMN~/pn8i<wY!Ej7Fi*A0&t%S;r0*mehTfv."^GfU;G+g_(9@x)98rVq)l:
:]6m$rGg1A3GUaZN:k.o)L:jn~3]m0Y"B.Ti
g"4B64fhv]oQj-SvJ%S5N&7k6P=&/&/Rqxv"G4r>7/#V/o5H$%EgIDX2w,`U_y.i|t}v<vCm!L"dNsa[t@BuS)"y@F{%74hxy_v;V#px{r)QVlB.H
(e<N*l5m`@=W>@P3bT_rkOl<+wI2*=_%ud?s^&F1c:#E6PaW!P^cJ/785HJ!!.uHA`hGjKkuzX@qHpM1/G:fFe#!Eq!aUh{<@I$2GtT%:H*9USeJQc[W]y3PEa*CnL$$qC4p;5>J]peFHH1@x8nr+3|4i_{Aw"T6{>@c*QyPFJ2brGiCQDBCEg
OmFJ1<mj_>HF="4}>Q]^Q(4uuL3y:rbF(%RUgcUwDYLmN38TH)_jI&Pm,B5h2HEE,g!xb.Po[L5Xmf]a-WslXjQQsHidDx99*e^fD>
yR$/UBo7)0je
yRBHUx1(9ADpOL]hr,*JyLR
0YZo57Ds`j:V:K9u/k)j-&uWyxtJR0lg">D"t.b!s[=uCT7ZK4ytA~(1[yOu#,S!nF1^hP<h8kJh3Y*njU&XNO`eCr+@A/3a,.A"6a)<QP<ZD6Zsw9+=`s`H.fpaVcFcuAPGa"2YCYa,F..Y4kV.dyf-.KZ{^fZv%:ZO!tB}+5Na$3C9OilPRABb``w%dR/
(3SYD?"O+XSdmSSn`w*,(xP%F"bmMat1/fs@)(QLs[];0(<f$=Q+_
+h`r"R=~8E%8HNJZw%+T5
IOM%
}$Tz(%ih:L/3Q#@<4)}%F`HQ!V=Wb8]%GKedt)ha*UX*`%;Pq7rJ_e2jd)x_Q6ipyQ~,[yfv;7n-bsS2*1JtE,citK$Ih<#I7GS
9XHP,Z/q*BKxbp)5_r.lPj[q[T09*V-0#YU-CGUUu@/w6]R3rvjpHEX1!4$D=C)!(d:T&L{7)(Q*sO~,NvI&EvvKXAURGRGZ]]bi:-r(m<~p=drrs%d9q71eV>@6v&jLP!ja]fZA#YkfE7Hn.@sa/>njq7N[~-k1N,8hQF=t;EBxH,Gv3]j/S:uC#;#V
/3^C@nFdXZyg+F+C):VWf}.GJCLq?Qf`j`mY9*L|B60Smb[!4JRg(N61A>6".]*!96>3$]QE0?14/??DH]w+Sy2x*0P;F9@&%_Qd0F
h69+31@AcFEh3@%v
?wGV]haRMZ<]*s,Hix#6`sC$KXpl*"p&uq*|J?<%w@M83Z_d).RCWr7RsX=UJP!!Y6ubkt?JEAirJ%vUuCjWy)jX!M)FBqy
X#(r]0C"gC2C`(O`nd%vGX.i-1A*k0M
NYMpS:)|;*T(Z7MLFh=5@qQCVF;f)&K}mRX)n!9.&a6r>P&a,E+D=z#ql1QIxU>SONn8?JpP
X*.5TQ{OHw+Job57;TCiS?dSt4/Nn>@j/^qTJ+W5b4O-mUmq5;#$".hDDk:$3.S^h&_&8D#A[U5NB5u6>omL54zr#yiLB0C;004NBm7^:7^U`chD_&
D[bR?GLMh1&t@Ws*CL]E550rA%`=:}*dO94O?lw$*nn%wfw$2d!@2{@@)_-9_$(@xvVu^.B}FhgeixDM@;/QW#@/B`1*IRD|=?L;<F/l&:0}
ii,l{S_Ya(j
pEkU~?l#jXXrAWLq*vV^DkErOP&%a0Dq"i"B,f/0k[eyF^9&fS+aYO(2w
gM<t-vdFdY
a;Ugj?mvT!2Ft"Ws*8m/F-Vn:bb=E#%qh^C9[3fVsAUPAtV28s0yU>h*.D?70JssUsO6>U9
Edp"v&.hZ!<qJ/;fE*XjX)ZrCZu#*xXy
E&`CfCRK_svBefG>Ti?1
TU`LK@
B8usx+(rYcM+5TbIQW;L]b%l;1)4.hq
{Rp);hYi/W>%2M$<GF5ZknOfd?ffqi=)CsC<QChwN?}i23m,Hsgu#1^$.IQ:.Uzc,_R^t).k}T-fqZP+
UsdN`P:oA%Q:4Qmm?=m2%fFN.oFx#!Zlc{>*,ZwYyfy6,Hg_H$t70=<g2}AP]#Vj#OnaG`$Md<%zWe%?K~-"A_pEGb+MK|VT2Hh0q#Mo"thB73PI4Q_;.1>eG6Wx])MKxI:a`UP`E=EbC%i08ZRun?4GJ~#n.wH-Qw6[rlMB0JSb6CaOjG%tYLUabdeto?-Q-bAoHK]~@"A8x#6kQwY~J]p_=}w{UV/C.Yf0
#F{;~NR-A7sV"O*_2A%n;u[S<mfq;8DSDo)l{)lLW-*s8:_)P5$G=J;T"FX@b+tv{wh9dr*pQN
d%47j3.[mQECK>K2ccNCM["y`W#jQ/M{#}NF87":0Xo/C8MLK2(BX3*4)U+QSSFO-qS"w>k^NohbLzUxiR.cc{mpz(q]ymv&c_8VGFk!;A&z.,li@+c(08d$0;],q+AS_zVd5#GEZ(6(b?q,jOd}<$(9.7<0yNV{N+3nNpd!r1>^Tx0SR66?:2Jl<Px*m@%Mu/+NP^qY6YqXLm<`Hny,o$n9_H*o33xEcT=Ei16EX+$Uv;,]sG:"uq-b`.tRK(PZ"=gO7(PSI@e9,80Kn1j9naLPu12<p.K/c1b41_k.W#w<eFnMAb=BmW_!=NL1FXm"HQjwm`
&^4p&78
yMdmLHQk<`|TvoA2KHq,sv[-h_n>f",5pImV7uoZt1[jM"D_,RC/mXqa>EN-lw-):nd7`fQTHBtufy[BPm.b*Du1hlK&lA
=Mr}o!UbWjN$Wdaw0fFtjXm`g,+Nr}J[j!=RHB,w#m(MFZ#eULGNooWp;8,cuy;3KbZy^qh;L>obq$/;Y@scsOM.)~R3F$TiluPw(NF(GO7{odc0=<sMxo!LEhQ"cAAVaXo!n^6Y/c#KF,<RDC+;oe6n$Ho7w-K8pBZ=lsufm8GgO=BY^dUp59J0rmS]F]
1ANM+/{6r@)[$
u7WA1vE$^I<
$,*t7QDPeHJw=%-Lc8MTWYD"lA3!{f7MbwYV@!vP(&R!Kfc:Y3700jJV!Fn+A2"iO8foN-&Z01w&Qb>>&tT^q"Z8
K,oX[)o7E&a:GKMjtXNirug3dTFAiH
P5{``Q%^u#]ZSW7HVG[KutRHF:HfT,^XS2EEZ*h=c&7tWjA`T5XeHaNb"]n<Q;!lD2>*@jM2jZlC9C,FVS,#>8R`1W5ubr}s;qE$1^#`:T5q7H@],@P!KZNEmTl9?vqQ1>ySA`Hu`O*0{$zNwww00joUb!bmwuwH%tlL#6FdNA5jPJ#^Y1-CYID@i_wEY3j<7Q`L<BUgT3SuK<1h*A|iyHm390,aI]"]y*3!*^^F~SF5Qjhx9aQn^["O3Uq;b-VU;kxL/IYVH_;WAPoN^lv;DJd"y0tOW=w%o
o)(eL];q+HJwcCHpLQZs`asRVpUK(@g
yJL`Ad,tm8HAD_y$NP*xe2lfatBB+"G[(JD3H2wAJ
hJR`P]Hd,
x1U[2=!NJAf%wWQyu"p[#"qb)"r?Ow,3~o
p0%v1t._%19QQ1Yrf%=C-wL5`1*zOlG|?1*J5,d7W"_=t/cp`Lu^iZvO*X_N!C(Iv&%T2.dm8(%f/@"W#8bt:Dy*5%a|c2/G5rM^EWy6St*XJe>/Pfb*&w@ctdB;cU+UnC<S4Nv~#fscby2]]H("1db9WOF
P}q^o^WKU;Z:kqn<l;Fz/[.9C]%#yM.81UA|Y>OUNw0
-?#3vY7JNPY6/iy#;jrjE"bMO~!>`q$z$1"B(&IKiuex-M^lw1rEe-?+p[KfmZ.2EA@Q[tZ3sj*-M$g0o6Wd<_@+mR%kDVL`Ukn!@$qxhX.*hz^<@a9_5=N)72id/xr)pVuHLED702.!l|yAr=C[P#)<4o%bk?3Bw{ulZs:}.HyLGv60Wy6NkvK3</%I!uc
<JvWUHB*qF:^cTc@H%@DM=Z@h`D~?p_kT@eZ$;o2N
eoy"I,*}@c0#,,p?Yj/qHq2Q2a45!yNHBO1&pT!,XeqX8IG7P(/y"~fI1bbSMbbYq
;tDRda)f@G)S@=ENMl>Mw+&f
PM%4shY8W?d[-COn1+b7n)evGA|J&ILkko!!RkrwuFz4`"~95^oB349^aQtp{l2si?cY)av@H0xW`24NNhct{U#%A2!brRS*$?XUum<L5b9_J"e=ZU+9/%PG1=fb9)z@HU
,?.=T#eR:5Fi2rq$I,fB?/d
g:7vQsgG4<Hy)2+!C3wh=53"-UjNv8aUD[N%Z%t{Gl8|Yd*LLRxqhg,z]ZU*SG]-O$J!JF/`q_*B^iY00b(fnBo4OaZ6EmTLOHSOb5Uj?}Dvl2"B=.a5g#w,:22CAZJoL/eSmO]x]!"?
8P;iNkD2j-42JG-=-DuagB{_[vNM#Z.VKeMA/#w.cI)$u$x(PPV/Y"YS82s&=h`!FNrSVv;$,3,gQd}J;.2"OC_Y44.07q;R@iWW]tqdpG](37zsBb}>>ok^]b5O<wi_xNL-:KKyM&*#zA+u1#%XdI-
$B2c7riwLd4ymy0G
A$)yPK(".K3$#v7DoEN~o_ur^]m@,%=)*!1`)n63$cNZ.TFIX=)nOVHz<s."!ypaO>;whLtSy7*QjY
ta*qQHny.ti6GVYw6_gW~j"m}.Gf
j^A@KI_kw*4+p{bnSot1bhD4&aeKr[2HVx(Rjd.w96Vtm.g8bl=>llK&<Q*fDCPpkoEvOp5P$zCH7rJdpg;D/MKiRj>,F)cb0168>0k,Ms7(SqG:4ts{W_d5oRYP,d.T2Bv8hMJjjGPWp|qQoPRCc&kTx?+WlEV0lM74Y0y=BIk6C4jF/q0W>^=F%Dv&]Tq:g<jvEOF?*<0.Q]bzcZ
?uQ,A0~QBw1`D[z0g,q(NP}fDT?F/T%fNW<-Bto8k$N9F;D!n<BJGNCeuS)Vf$jG">pQP^/W-CLN.L5gm%qGBeNa8XTFJPIuKg8:Mh13T&5:eQJ=DU?Wz+&+1eG.#Vlx18q[2^E
512&_6Z=}S,kGs2dWNJ*Ls)BK1]__:yW]S7#H::rhC5g1,,8Avz,e[(;}i?"Z*RxeWIG3_4@U$n/-bf!TnQF1@Z8nD|o_P~#~eST0Fd1mqHS.4WOS4xvQ..x|aH6=$d&?BTI1EG]|#`2*Ke?&f:0gFvu{2XF@b"pao4&^eJSiu{--a/wuxfm7Lg@pg0_uTQI(VEwK7z;mI6O0Vm2AC4b$0,bN-(G&NoQ!FcU)KP8_<bM0K1-y4.K7W>>^cpO]YRP|hF=BbwUP>+c6jM?H/H4#m7?Nh,_1"Ay}YU2W7qi+k<1p7WF=(m)-O_hzV^"N/e>K
qPGktD*k)B}HVPU"O83m#,,swy2uy-Jqec3<pBTm{0s>l1YATny/>B4J!&8By5m]pv>uyfz4c]iRI6..a0KGKF{X@Ce<QJ+7o1[yHglIYZdpXqHkp]:$htj:EB.#R3lBQp
6[Z>?=HX&Pk%*g!;,09~Znv+9Q/KJjj@_WB!4B$yV&*V1[H@[2EI(@B>,Qh}w^.>OqxvF8!#$!0fLX
cow[YEqQRG
sE.cfF@SF]v1H0s4co+;Z|p@+w&Myqn$[RA/J-!tF|Z/)w
/I]ddjWDT_Z/=%s]J^I_91rLDCPm9NZJT08xxuFhd]Ve=Qc^vla3s8th4dy21J5i(GTN6F1m`oQc_&CrXO**3u*?wZixQFwq+o<
9.;JDw&V
#HLce3+Ec}H#)j5*ZJq=c{XjaLkpd1)9D3j@T}1Z.BO/R>b
IyCE7<3^:n$X8i>!o<u"C`x|N1h%2Qg?X8U|g8NG%kuvoz@}^kQZ.~C2!?vQ&k]2`$cJ;M_%l[&F0NCQtzk^2>ngiXO>S7Q@$wRha"k3hmq,F4+.W>e`_4Sq1(NhhM;c
0?}j`+Dg!aM(PA-x8AvEv(!
&Dr%gk&lWMVw2_$KV)5fBjH#>j`:7M5"u-H91hWt)
~:O]gIS=qe)S
Ugdtb.Az._u;wY?a;*M/T?U]_buJSpfH#*v"OmnmOTHj<`RQOLQZXwR94#Oo[?h{>Y"fEPKEEy>pA|S??H3Hhq;C[.JHSO.%`X?D^b3T163X`r3Fp%?N4=a:
fZuraa)+&@S2kLK4ylH2^xvlyIcRVEfTAvc!?U@p*SFy]Ob>ai+Dk^cs>Alc;B+C{W}^{osZ{Gg_rxaHl5aO@Td:MULF.aR>#t
L]u7;V&he{XJ9.Cq(@hu:zN<6(Nv?{gRpx4$fJ6FqH4K=-@ejCE8bS6JSYm!(+UPE/!104K,n[fe]+Yx9T`EQWb7h
D{t^W8ZoE/]7S@2g!iV2]8:N"sk]ClpqP,:?#nBR.N+Gey-+5UL2+dH1a|0~L:Kc#gOkTgT_+]m?T::|;^0;
/nnGBb9#O4J`C"n=dq8dh*OR1U<iMu_])Fo-yl^SYV_u4%e<2>dkeG@2uitgTS^C?&Ujp5%nd:.Uj
S`8[y,79hDD,{m<o5e[]pG+(vdq[7ItY[8Y6AbS
d:n
qWf&yq+Zn17))N#5_wiFh`gMH:~@L
q+oGcvD=#s2gwc4f3rzv|<ZG&h"g$$A_g>eALk:Aa-ZhZw~
]HqY>j,Nqh-ks-W1uKDNP_gGn4FW>Fu:JF4h0Lwq.Adu7kW`LbqA+&"OWu/+rfIv21Sk`kVCv.x1G/gu2obRlpza]dYM]P?mgsXbag?qcy6PYk`76_RvJIt-QXiH`M.eAIw/qGvJE>KInfg<JW*u*C">SB(cIgo-.dPrag:GBFw5>mOj/b!aSs&]e%DDpIGh&]>ZgS6ZXf-
?Tv%&Z)+6<G)%f._c9{BVIji-r3%<fS0Imc1CTwA+;vOQm_;n+/iGb(J0ff8[;_t-%lB;qtrz1CUrK,_mf)?84kp=_|V2p3tTP.L@mMy!"Nx^94_McQEa;A%>V+-+%_UV1FQbo^jyt.kJqz3Pg/^^ApAB?(g;P?&<9zu1WH#aC>N+^iDpHx:)(LY,G0QcG"*@M$EV:xs$wKESt~,G9GmfT5)o]kEFc:4Y)|1r(}9"lds+d$IQuDtOEz/~Wz,!xGnEDO2pqw/5$*"N9hRl[VPU<:n."G>.[J5IiFZ_AR)5xH^D<406qjH=8[-:-=ID:j.I*)y.&Ob!a@V`C3U(elC9"6.{$Uuw#N:Xp.gn]{^dRqe<D-Lt[qX$v+qK./R$%j%%;HSGvdC7-nl?kdC+)XiO!/UCW#+Ls7^;X*Df%<-!W*Oy?dakk^Yc+(yjL"db;>,(adkpdA4c=O/<^pV!/aOXNa:K6
]N6{$)"24k/v1IT^"]rF^D]u%x<:j}qi.F2:Z2CqeD?M8PJB%$%3QGYrL(^N!v#?3xxe!Q7$8cdC%F$!Og/g4`RWEC_k;fPb.fk}3D.*o_McQ{xd#pbz67Bb4FFJe6,(aN)})K
BB{Lh&H]u]`$F-?;6,H!g4C@-a;&D8~bO)f:]`2fud4v@ti^3[T-?!+:O6pP~vt%el[b_4uc%9G$eT2D]^BSJ,N)~a1e:a1u$xnJmFIdX37gK)?9e?-6-QBv8"vt^YcDYg3Cre|F:K.q>cRE?e^&Yv2#2C)9Iu2X{i<_!LgsqM9I@w$(n7
MBk[vOt7PDm~=5yvc@b],AMYs3Mxt4-jkwlgfiHmtldFwpQnqRtS^!Amfkp{URZ@w:rqF^&`R)GoDl<m[m=/wUKwJp+ZMm(Xy2MYrJ*YslvX`XL|i;O{MbyayrJ6=hqo$_;qg=t]$<!LaMQltVu(w0K/R#mjc.b1QZ_t-K4J,7gxS|myeH>WgB2,d^kdM2;y5rH`;=v
DIM$&N?X29([Ws3PG!q}%Apcm|y&EEZ^l!RyepT_GnOoOnyR->s2KLN[4oY=Y22"JUHGb]@}bcl_R?
qVwi)PE9;Sl_(m8Zj%%+ZG]^KjR3jErgaOuYhAofbRn7kGzu{UqYPUgY)+H6&fP[DA27bc|P>v"V6e(W9Bmi[6>c}YGb[d@oDvK.?o`b]`B3/:nZxkw`%pE:iYAKu&XrDz"rRWRi)BZ<j[V2</Fw(yXwuoGH+ne5{ug#5k(O(A*a:E*B7neGgN~Hl^99$KAXosqNMtnn%5(*T(]K4)sUbK/YFE6GTF1V(:oWzD>W4V$Rc?:ma-7DR5$knFQJMeTn+LT2r/@%es`L2GkEC#pS=n3?)?c#TT;rT9&+,d9g9<n>zO"l2d$jD`vg~c`K+!z.wxzP"-(G1JysmL/-.kA3v/Vkoc=MyYU^[6UtV"gdr%:Nf3*>$SK[BkIe!@&`pw@nuLXtTVL$da,MU9DmXawqrl?bm6_Q%kqui[aXFMny!WN,_n-!er131R{]TV_K4&}yBw+,Vv?n@#e#;2Pv-GP8S4:,my8z)K/n.w7bmu}m:6[29c@P[K4nbK@nCXAc|ms)eH|h[7?Q2[H
8^BbzX.KKtA-aEu[818YB"Z=CDe>/b/hkju[]vyZ>/xm=b4eT#ytutWWk@FtAcZS!cXxih3`+`fndffuyt5jmkdk9OC_sl5ERmq`bFg:h<XV
[i7cHf/bPDTtK4n}N7k55~YxizP"9v"nx_K,qw7zMfs[%@W3m=*a)ykS46FSB/68)2`]4#oDQ!a=/bkGjAbS)Y@~ehP>ZA=pX1To^txt^X3RSrU|"U6]L=ym.`Si6|ZY_ts,3E78.`@Q#X3!=h0mOK.oO.yBo#l^K93rbKq@7JJED?,hF^/8g/OX7*
cb]xXo?)U1oM3s{8O_=xZ95nuKOa-,w^b)1>Sp1PyD(kRX$z%^QetxZ6]<"v$vm_:!$m`X(RiLWw?yrY$)"3^4<]]XYz%/dpfS!APl.vu/^_1[%8ll&NL7Y_wnMxc;Gtn8P
ldlyblM_psPJON?!F]e;I2~D8m<Yqo#h{kIK//esYao@:*Vu?#=*H@bF_d,dpY^b1bI!bLXwX:"xbG,c_K52-4@VEC4Gq/`.*;X@{MZ*rY-bzI>_0z&f30*Y5wPcDg7xjI@TSdav_>njAFVc@?L;<2*:K0o@t2&1
PWHK=hL.]e%V:Y(iJYD%9Btf!/g20Lh4N.xFNP0C"
!]bUK?q?h`gpxaxMz!-}Qq!FYxKr3%51LJOHO`M
tACTAGNP<i.][S[[xD,fe
Sjb]_pw7u-tCm?,8MzZk[Yw8CV-e;ykgheLA;-T[Xm)sEEh2up=Aff>meV<GYv)z%EMC3wmrP%E6g%0X_fhKD8s!gLtE334X3LD!@A<SCeY/k(WC4w6E^<nal5({rfV32M#y#{aombN0HWjj]D"n)r+lWrHtBi.u,^yfn+:?_SF5j:a4tY>|-E_cfpF+GTya"wSLL8FV9bTrX(yTDU"p:MdBUtAd#lRUF,_B$[dl7>A3%93ds*ihAT<R]e!XT[<W
@qGIlbZbIcS=)R1""7t/HY2KJxSb_s
/)
J)xeGBh`{&Rf^$#6EoN<FbQEb8E&Ln-w|,e[:Zui2mB
v"/8BwWN7A*AU,g4r"p7Di+kRB%AUYdFa@AuRi$/ovoj2r/y?8uw*ysL`=tFKGTYXU$E%@MD0X7o|cBlF"<K.4![fk<":*k+=/1Jp@16g.^ypM"E,YGK>@i:ZT58o0=F[5rhSt_,=q|c_10nb5{dZ(%Q{t=,oC=)LJ@Ai/~L)puseL/<w(AYI8g`dtY$
2)$./3!O;.tIN0:<wrhj
~6%cl8Art8Q=TdkxS=<shCf63waa_L?]]/,Y%3&gIk6afCXfFmR]b:&[OIz<Zl>t&?!>eS^7Pu$4REJ
#aLP!4Q-(#$A,+Vhha~5IOd[K/b.KBua<ww<3mT:I4yj-D]E#Wrr+#r$0%n<B<qC+(c#K[Q8_2!.A0;k"pYX[+LSz?xNi>7AxL#CCq,MFk3f+D(TE-
i"2_^F>e/G9g]6J8,v]3yI9y1H"Py!.Mx
!v(R7*7APr,[UUALDoEg!2uI8KfnH:=iWg-{WZdce[
:$Q"?x.%i3636XOiby)-?)F5c9d+^>?e>0`_B?LPuu>!ryPw~7qtSdYhvIsK]cyojUF-hVm+hcyOHBU8.vlMwr4x#J:cDduyk"j8&*l%^Q57(I&Z>X%U,r|D{$ij]Ekf+gI,fEVeTpJ!>f^hZwg
:N8cmg7rLT5"4teEvezt:OQE+#{@XKPhvAk.NH{*-trP}Y
QAO{7.[%-6`|Z!"G6mTRSSG<q{
8Hkjv31)U_lZ!<F(fc!n{,p3`s_=%^(D+K*`%$!C_!??[W$mB"
LU.j,23M[JRwU/]4VD->pcL23SV2PS74:nAA:);s)/`<>jvfXp^l+C8E95>A0X69f]dE^eQ
8;-bwx<x,`Q%pQjWDKBbVgM5QNvi-
:`VDKu,-/G.uh/&q
h<~w._W(H
EvgZo*FTE=V>o]~>&06P(,j=m-,Q
3Y$mKq%+s}U{NzUCj>;!?dH:&jwC(tS*,W*Iv$?~N/ySN!1H(Cqm3wo.#(S,IZrh2rgE!=aU<0x(o/"(tz@Bu3cAYOy5dwNu,b5{Gx0]]drx8F+%X
8;TLyBtr"`+7/dfa*hT9;D;.=2ti+w.yKFfYS^D<MEwm$;!DDlIKv]`,U}U!Q+3wLj")wh^L8>Y*_>T9Wlv5bYnZ?P$GNb<PK,@/<Zwk%.`g2ZNkWH[F:UjxZ>YX`{KZA9;vvYb-2!,AY*:
jumhsg?<knNx
[_N_hUtj"i>M=f3"zPf($Th&v#],?sF6/j<Kz%k1mkD@/AL%zNpgz<JuR^/(:)`EFw)sxo~siJMU=Dpg@bc[;o<ho6d0"wbL5J,m^n>JQ;=&#ccw:7.#{K5ysssn
KNw0,~Bao<M+]$#XA+NJQ_s89tQ5_RAto[YH<YQmFj[w?`![8sM#??mR(/l
aV_"lCA*;1?sgs!CneIHSO^_?Q=tj/Q0r</]ge:`-_TtCwfx3$T=INm?r25xhb?
&!<mvGd`48[B?Ty:7ed"F
59mZ3xy1_8AV
fe@*!h=%Hw9%gRcqc)0L=ix;9O"Avc)]]WkM-PY&KTwA/>}Mghq>`!)y
G+q@yKe49Ex>K4qwQs5gB4]o/fY7a5yI#XZ(i.t;y%7`P0/sf1>lfkkWrWDr%6Ut7GfrG2$8HR+(L?0vV:LV&"L/R}6,m~;PRos3x&a)^)4+kL%2nRtWHKa=!g_|SLpjO*k4m%65K)]y.4w<c`[l9`Z?;aG@W>s__FQ;ZHa+lwd?S[[5[=G]7@PNuNZ+vTiAm>0O?}5YpS_-Vp>zF?)WrIb?5kRLpM%^>IPF"(>69*]ARHx_i6?9MZEclZF%J.
>5{jpbHbbAq;([AHI&{^H*0VMv[BILGlIB@`"4@Z_+SbfU]KxUi`Gwg]U_o#h[uCGQYn%3JTPO!;
[h2MKZ"}Og`8XaXayGCkKE^+g^YEec?.Ks`Nsi.VBJKGB^FX8
pP14G8A4j[?Q3`Tt`P])Sb.3"k*1nvalaN"FJ&wDt1D/f~qD)2!>+mSlccVo:oxeuFTEAp.*oGY$Qslv;@Fd,MB#k7wP@aEH3Ib#^d]Vo03Fndmv>+]41GGBX!j#TdQsa<5H"ic)!#`W.]Ox"t?Q`-o%
Rke*HF7<*mr$[ldwDw
0uF?6$1Hc-tK)GE]4u$KeS5RXEl"5SE/Sg`>9]Nf=ZGUszsQ3h3?h!H*M_P,a^hHCK(mWHy`aJKf$@H$!^aNUF3&Nid5R@;7xP*BLY;eHy%l`KQy.$Eo5"lYxLVYw-2Uk1*DIzu1X4`^pFLZBMmj87db>;RCB|El)Dn#N)izWf6d^U8{^E5j
LHJvq9ER(j?B[:Kj8K4k1K8B*$aK8_HM`2*7FoZsH*v]h5;LY@~1Z>j,EPDYFu0/3FTA8x43`Wm2q,X9iSVg:rwSNu(Z9tw=TxSEi%5ih9(<Nmj,v50x[i]an;C-H9}"3uF`*`ou8Gig4MNFw^Pwe[mCn]uShhjL+x^m6p_"[%[A1>.UdPF=$UE
Kz!Ff(6uw1iLrGZxX]=A8iE^6*g.a
a@5gpAJJhK>V1OtP,+b7b;5dp#N62Hf_6OVYgq/4%
)C^yac[Ul?k.c9T()Cc2W-jQ+EkDsaq&l<sdH8Am^7k8n#j$V?boJ^=6!<R"ugI&g?.08By5/*^`p;TkkXYgj6
&*"@=<dRg-Q<[j9X44Xa]s(pY(X.$QsIQ&b,a?b]emm!juwdwXNu05/aZnS@$!_Bm.&r:A>hMCk/VAo62tKt5Kx^9%/EE3SnNeBbwBV}n&?=]sIa?iIH!m/po:TwVvbf6y+}l%
C;IhY$;#_fmk<e@8swI%qWqhZt^3%eOw$5ogWtR=FYM=Hm(tbT
3*$w?5w@V}9jL>g*%NQ7fC-UlDNHQz2[n]1[a"50<uyl6X<)rR1$=YJn$oo)+,EgdOt(^IOpOMtd1P#02Y=ChP./!O#>5=Mo[8k<dq?lawxhLS&^wru;C=*;#nCr@)Y>HP7#h2nAb:ccs>><YVURNkl1K,dCxAo0.csTj+7[6<SCy}<wdyc$dQ1bc{K4G;q!aR_R?>WA>-gzfdYM!JG+A}dc6v_P&}[p01jjqV+<cfl/Q%a7fuBcRfd?h&UJ)JL=*I<Sw57`r8$XK3^dO%GyxX6Foa0()IL*Dj`R&&G
rb/@h=U%7a,.eA/Ym-^<=@
OAcCG,P[ll`
&FOb%+(i/u|r7H01k"l`myJnqXY8O8qZkyfG]L9nCpKi~,A!wNDL[Ddz"Efy(Z9!pK#2+)C*(TBNm;z5RUYLA)"QnMM1U6C(qg/Stmo)Wx1#t]tBxeFz"-0p["[3q1=(rHp%;#_/8"|pPTJ-J6y%Iobt+kj>JK=*]k&yrmiy~w^twu6l;qloTl|RlKjSE$Sp-cwR|fL7C_b%C=i
>fMMLtQ)cSi8Z5vSE2eo0p<95gkQgL&3:ZG<7tM;[Dg2cyS*YKT3s!^`VJkvwyyRZyssrn?n.sC;oJvg"$]kxLUt3OF@Rtm`@->,3iI8;>28?y_Ki?w6UV[9IXbVLfIoap9wFn*v>:yl^pxg+I)y%-p4WPbj&Rz,6X:i}ABGxTb%
8IeIo&!PSehY$[eMX*NXhMC5L!<Hw!"Clc^*#Rd:<QG?!(K,s?-S#ku[)5K3%7@;Y?7XIZ8TYR012cyxBUz%hVo+2.c6:-DLeY
s9&VcxgX#jq`tRs/TK$9LNxp==a/cNQK=d$s1Ejc}L."}kS
2,K!JBHoZEPY[gLF*s!xDRS-2VH499(yp*RmjLC[kz(gwu
gq/hlbrF:y^oXBw5&7->wB9xqRc=K"2#iA(Lyg+68WvFbfT==Wn6>[17(wI$!l7YY20i#l,
x!#!1b8{>^5;C&G7:(ka4J5K)O+sMz+I]E=b2;-!7xY7kHOjq
.:uccLc
Xz_-5ReD;%$_R{f5tJ%qlGBGyg0B67YMco.B!jmc;
T~#3-tkXZr5VhHlMIyK,qW/{@t_#*_"!Gw[7N^m@PE$;R_b+q+<B=zyWSG0A$M.(O-ZF^6[_$L<=F<9-53%W9B1@DImKKMIidB&6B]-,?)$`8tYZKvpgf|r::F0@d~*fO^HKK_V%`JU.TxCcE}:@tk`AE;3S
"+Z"Lk=iGI):<p.u`%Gs]gv)}SXamcg4f7d9frEZIEh<L:1$`86"$V>MZ,(H:Eg6~r|GN8gjz741t4VEUWQ?^clwkd}nMlly3bHZ>M5:?KtvzdPKFPXE&edm7^u5o8@K+,q%46a6bw[wJo;$~+=W.t|=kfKy1:GYST9:(hU=M8TGu3%1[%ZtG>|"E%Oyu"rY<&!PC-
$ItX@<
yXL88eAPI(V"|R^4s,|L_-.y&kKCgVR<_tYrVl:q{*yN1>Bj#xKSubq"^IzK3:KBn$U0uF?DW$&2~F>ggUQd;DJ<Af5#*!Rpe<Oy
g"V~u+OmOG"k+(HJ0dP}4@`b)3<6#(,L=iox(y=[l?
<xiel*&4p4Rg"!-eDN-Jm
5[c;1(Y_"W{4AcxF@Rn,7Extw=kPytf-#._Ua?
0m;OT_8VTw[<r_Vz"NOr1X7re[j%1;kaI-Bpl
Ali]pb:MY6L=(?^U]u%-:RB%x_qcofYru_S+tO@(`-$tH
>K1t!<OEB^Mnf#Txjmb*NQt#w?Dh3B9@3-cN[(11h}RhUA9qje),-?6IZoSWP_^^<Ohfe(%^2O3p2t)S,%,!.M;OS:;aBtT.-Q<V/OO;({&(+ruX59;9+!mU3~CjB)Ca&<bsY+1WBoA1/1m/F0ho:-[b2y>X$?R`u)$pve!C=8n=2n=<JJs5,$Ct3}?}9T$D++N5XO!Rf]HdecI
!X2"=u*=B-*WD0!a;Hj[2[:j,4ZZY
0.cyo{W(q%eSD9!}lEiZ<mwSr~=ms5@%*.e;Yy=W#E+B*gmj"vX&8yszOkD%RuC{:;(*Rm$viO
EexWKSB-i(_WO={T)1hc+;MX&I2sB8CyDYLizW7Yu<YC%aY@}tgPge"hMcPtLdxE"xQ_XXA/tI_wM5%,k$oP~N
CVt]Ubw7(x]:
-%ry<$0.WmkGo)v.Z_wG3"aK^4<t@T{OY&+ES_&$t6JYTL3yghhio,[RsPDA`0KnKT^[PP.A/*S#eOD$yl<(F@Ky[dP&jKU$Fjar?U@klWKg]JsU/4I>7Nuc@XhM.D"_)V6?>eVGa0J$t3=c*>xI4+UxgYKroY%;lCvcFSyI
r]S``h%hYw$~&H7$K@6_eQg"qwh/?6#%,MSWrQg-UF`sx`lHWzMTtRgs
5peK[6
U0KVO4_q+0WCp}s#VC/:3yWTh`.(gO`xV1Jg
ObY4N[#f5Je"R%
q]vl.:kMk,%P_[#tdn+m4#L,G^3<qB,b55AZI]gWMo:f3j^|%~P>60j-#S^|a39+Ym.z4DD)DD+@al&/;"^^qcf|>pD>r&1aF%b3UDx$pv-xAj2$D}7NA}.WnB)Tc$UF/sp6)b7tY
[I[n6+1eEG;j7w)#Tku1@|^*RkG:1Gj;9wp}v[]Or>3LT@hZ[pklP^g~ax>J#o/3S?`UcA_=uySM_A+aZdWkhJ38RngMMa!$?O,"B.+7<$>Ja.X_ja)X25UNjo;.n5o`Thw],zO10n&v7xb~2HXR&>ov.Shj()w
ic%vWp#H&4VA&x!;`w%LJ#kQ#yGQ)$Y(
qruX4iIQP^s;_WUbi?e"v"srrY"Fk]x
a8A.6I4J=etYv,c<0fjJz""%zfC+[no0K+v]8,LO}5]VNWePK/jS];a]t^Bu{x~oBSDVXd1%OJzgEOw0Y//^we29=;<d
P^K0._&wiV+b%D]=et7Wb`gZ?G4McI(ICWPhIx1;"n/=Z=U3dqX*TAd4LYY~yim-8dI1YA0G!W&>="w?%)/y6/^e3((Nsw.k#Mh[
MW|#xV<F-/h5f//&ALsf{H!js/>=UKa-]q@(xLsV?V8#*`nYXi3T4wVMYnlWoFvlTX.s2+k$-of=Fm_O;S>Ab?_!lD/_?MGSDYY
MjcZUu<D+V33&e)8"lWjc*`iUT#Se=Gd<4X)i2bB
[H+eplu60K?[@5e-k:
6:jHT=ctc>jS<=kQxN4nZ5Ak
`(Jr&=sWiP0b>oE[Wvv$7X#+INX_cp(;#i@OD-9$$oZCQBB+)#c5/fDsB1Q-M.T/a,M-NW9bU-R<2s)SVW?3F<Su<9a]8<H{8;a|/Zor90OGQyIrKD4Spq%Rj$Yt@
ua&%$l;/7Bm*b]aYq}x9:~sn3anAqc&u3&,[WB?K/j1G!^59V}1BFhV2rFhTNe?]y("@o~"I?(3<rC+I;PLCDe42ON_;gaV|?:$R(Tjf6_+Gvv:KTmkL5,CB.TQFX86+0<h|;I<K-6o.2aj)-$#Cwf?3@b58_4+s=y>c@P7-k&Y;Wrb>sYO`essK@3mJyOFiFhUCxgW10@x##r>5!1uPXc15!sKv);dQTDj>^fDAV-r.>oJ!mA7Ax$g!S.IkTq%cHDd?SRK7wxD_y8kAK
/xW5x9<l(oWt8?`!0|qtVcZ0Gny.G{grZp9}ZwB80He1>2sO^7dT]CAdCd:"KRO~.M%QFtT
-y];Er_^3]3V;*Su
6DJB|vi<"*Sej3`P"Y/rV/SSd?v)
hTWIy/,;>8(ZxUDXl%86nbPgvm+j$q>r?zMvk20]GgH2Bw-N,>]wJdVmVFV|WJr2#=Gu/CNxMG*L3@QJI}KpSM?<][tFY9_`8YyI^o,u!H9AN@N@y$Ab,fuiHk5?=w`s-Y&-8FX=2)o#x!/#VGh&Vkq):88u^]P_)byXiWKSl{PMheELd~:YR7:Qjmt"S+c[n.;|j%0vF-Qy]BTyz"gl01]&
Gu!wB&9uR;|5=HN]0c(=d<A0X4@I1<OS9lfF;,PnahU+
".2lPUMRs$GaGka!/H,Wc;8AJR51d"tdNE@s@d4iRCc{D>jn
Rc
w|mE*p_F:wL_ELF>R
"a=|fVAwsjAE#:3(x5>qOpE3Gs!WYnfrf%WrGk5DVfx4-)dwh;x]l+"d#P"s.$cWY2bp=V/|&/Xz8%_)#MM0[/8-ZfyeHqPuGr8|sFdAZooyt$0@N(twPJio#_C%N}u+FS>_qPj"/LW;AC$=6n[1W-_@Q>in`;1FuDHI#x&&20husVE=K}W_0&jO<V&FEMN=x2+^#fMm(shm:
f0Pr7wgQ):jHM6w]ovLpe;g->_c8LKX!.{r@CEI#RaRscmA19p0j@M"c?Ci;T8d/K0g1C$8]k&t:_)a(LYuZh)>sR88*F5!@#1Xmy+A!nLq}&Fo86
hJ6t_5r=A!k.Z
I
M|/@IFw^AY8TDF)k`~t]g_N8F"+0m4F_5%F,o~.$R=r~Ou`&)]_)u"@S]_*J*gLY<
5B<EMYd7rxGTV7pHPhU5^lsW6NR7d&>LaH4nuyj!M5svs9u.B65q*yIcA~oH+pu&a.(aP1Tn)UQIK?%8pDxE1+pLw*6%*63}6,Q1C($[BZf1rR#(a1L?ppX2:|A#`Pfm4@gf5DAWPBYO[!Nc_6&8?rT-#Z3)j~"nnRik;5lydb!Z7,w:=8X84m7~/L:=d(ua()`,1A:l4.3GZ,3?IKWg^loyDTYz
0hI=!"lS"
u_e%=n5*_i$V
J"Hz"^AU]d?:($+J4)wM2K5,[5/Z-U*q;n+0gS)bq?V.plh=b;:kN5d8
272&HYvl<8yLl
FcDk>[2")d0c%LX+nu~9ihvPoNl%|<BAjp-XDU".7"*9TM_XI8&dt*KT|6[1"9O$WV):#7ri`dW,nZ+2hQL]uR%<1%.7s2`jw61>IKaOh[o;O(z;ce0xyQ;Q_84$QE^RUk&>eW;EKVGk<d
&)w6rjuzyA*#7jE^v,;)hGGVYKp*pP*hCpppeGS}iyrOKH.""sc%l}ryd+q/6_OeEL.G][4Nsb
]HdtLd/;#xWT
Z/<F3.*bZO
9$43oryG@cPNH*r/K6&;^1HkhN&:&G7TAK[a~;j:tOQ"M*]wvF8d3)[ktE,?2^cKBKh7Q-=C1%1*;iejt?DI$$]LRWPJ8_^d+$A9ni#23P_[P>!o?^QIy3})PgVyQbdw>Z
Q+$7Fz1YM([/iuq7,i3jlH)"@kjXwt>Rq4JFgzX>%xCg!|uUUjOtKfN3hPp01jOr-!Og2m%_cZQ:LBdtg3yjg:-W:2FDg[MF/kJ58&A^aq"#4PFqOg9).=o^fSZS9D8C,tEj2&BGm(;1SII7:LX]
|ow?,V56{M&Z1IXNH$gHz!gpr!~o[?@G!^q.
F}YWx)VDi?BAhme*kN>n[k6jfEp6;oPbok1|8rX%>8FNk%n0t
*iM+6|fVSEuCF,F:VA&u49n8Y<ocA=B{uBkYl]M8&|29ZQ#64.7aNE]Ia{hCb~2C
8+KBsp"gm*h+HN*6ITR&?&.XF`3;#Nqq*IfVmNB6=UgTS3fb/?MVd6|tL/+h`t.qHDe8Anp(b7
#bI?#0h=kbVyr"C=(YRXN;p23P_17!]YoCP9tAk2Z6;/Crubq3PwX~Rrt8t^L5+6<K0(x8W#YR]6#eB[>x@:ORNwK]tV3~elQoM+;Sb]%/;=
LS@b}3um4lry`%1>q1KVgFWO:!q$^wMso?|=VTKqiL%@co_ST>I38rOip/mr7XzHoBB3|*y!_rM@ye7/M%oAEuY?+x@Hxo~-6#TZ$?d>Ic3Pd6w,,h~x]
:1ks2;VCF6.KCIq+*2zu>$q?d(gAO:49^7Ok5oE7=XdU>(;!hL%8W`$GG_)y(SsENhWdBaji^0s3MZ``/q9=p$9/!Pl=![?K?F>EnIQ873J!]Iye}EU
j"*%0C6kKV+9~$hN])2DR&="3A+=4haVI:71A3e8T)8%#eE_j3=_~1J62.8YT9sTE8xSpOvI@^ddqey*.D>*>)?b5@P_kV.QYM,Obchk8tYmEUE8xY<-gl@NE!
T$8)hCTw>|2C$jU#npQqki3Hb4p)Jkx[0Xo;
AZp#q[<!%.4q;(CLt
l;M2EtqT34@,f`S7=1I`s

LxfbCfJhe~7AT|<A2J/{SMD4!NKzL`@^(Q?<"tM_6R*sj?cOAG:1F++!(4,-?sO,J&Y_fw>8$W:NU?2W51E+5/;5pp.q-[Gch6C~=c_nlORtj?F
Vv;~@|IHCR[F.rk>JLSw(BizrhOI];1~E=(cmI*BPe+Jk&inO_q]"v6OXQYZm:5kRTs@ZA$<f/4iweL9",pp>#p.EnU8(f+1yl*sXjCX(_hk1WTyC
QX$4izRIHs:BFWt6piZ%@n))5r:>"zOZ,@]G2k9;yqo59ka]s|#|gb$*Zf>pl([M-wqXj@;tNA&mqA#"=uBFolVqn!)3U|:jf0lUgb^LE5-~K7.T@Tck$6>UG>H$<"aqU<+H/kO4:0tvoNqAO;6ulu]u<B4[;)
%#{?6=D)"JV9q/TDN0+v$4zVZ=
KZei((S{Mxg)lf"=9[jBb}r0X|@a?I7Gb)G=g"?Ze>"&f*$YqtQKVnic;^pD8F.Y:DE{5LjC1_0th.Q%+oTlAkj2pNp"la^YA+3a5TXKN?f[`n)"pPuM,y;ND^<mP0/!PVeXDvKB%%Qo@@`8qy=#6qkKgLmW%+=_5tLy7EbXk+pyX:k84U(2A*0aC6:|+51ICvi&[/ZobCf
12JaMH"yWXiei!)j*8D9u`KPH:fuXJNR2k/0-Kt`x6;{W*-kuHe}n<42OP4`QONWYDroef:4D_<wR(E]WAXn87&_bIfM82,rEUVb^x(U+)Wy`ZVn08UDLX[^%*1{5AG(eZG^]6[QMu=_ms$%$LWi?Q_Wl6izM3^ej|Ou>
p#-TO8mytQCo_u)G.
`m[c_0/=d}Xpc#J5e{)y38]%E{Yuc]q=]4RMg&@C/$>P"DJxF|V-=m/0%Y:"O<DX9ACw4sf}J1W7IA7H,h_V!jb3Swh:8
Q(g1>>$vcK).>f4]-.bC&N1S@$Y1q9ZLdw7TTU:$v8B;WqJ)+hGoJJaqax#Q&L28v9!9ry#rptOnpg>$kgo{;aO+gnS8!OYzQ>EK-B!M`
4_/-8S&T7L8?(O1Js:X3^:G=%c#U=^M>"U<6PE;=(i*"y"&:^&UrPa1"Q2)@?q)0]lE^jJ,3oMVZ?d)}>B9).uxnpD7ex)8Kv~_,->%0_Z2Mte6NCa#tE|o"q~D*g}pMsGi:7JRmoyt9o=ty6]a4hl[gp0rC.,)Olj;,qP!e[:N{"s^KTfo-i|UH;wg}0~bH;wT+CboVh80y,yat8D*CJ.u"R.NhIkU(`F9K%j3}8LA(a},HS|9.YNoC7o5@sX^4A#X}:/`.!xPzTW+V(reca3!OL/#a]IIP&L^j:e]m3H:X#2r<^jp(*=YS<(FQO_G-j><NThRF!@!HewPP"dWPEsNoI/Nhxg$op
5ksVWUl"(q@/oE@G$4.K8E$
9M_1)>aa.jb=Oq7]v)]]+Y#^ut[&$k4SLx1>"<9yQhV.X6%%#06w^"Un-anh9Q>~I$]~08:?Az0i0`0p0@J1T]nYd
CNMEgj4/jkX}AG3?>J)/QCfdQcxLJGDX]A>J.ZD.8MmIRv>@pf=k;~F*
WZC4k<,y)1O`TA(:V.Mm(b*b3li%Fi(p5(@Q]K10*I;)fKOCVVh,xia.@]WVF^$NKR93UX~=;0nN^p!E#DV^gNLk|Z>YPpt%F;W64Cof1WFH>>BlBLPZs%{F9HV]d"."q+"5I&eA3;ul`,IH4.p_:h4YqDRn
^;/tQi!h/f)IpKGvIl):2l:J/=Om;BrxIkE
,s4krm0|0ov{+d.L)?@M8p"^dS$jt9B+?cl3j%^6E2vkEL;

L.5al
&nws+b!80q^Su5}/d8DQ*f|M{"!I+4_*:F<
r5yF}IEG1XT=H^g^^hMx~<&g1/3WVRwLf,LwDS^#SPq.gV*Nf.jbZ]fHq-^k=n6[O_-@fm<+;_6FAV~EPXCdU!3h9"PQ1=d)4(*U2unKiga+~Gn$9Amds"rPa3iZuC!Un!KtoU$4OVN#+?;6w8~0,frhZJ@r6"Thw[!1t/+J|s`LO>02+C:7sS-Pw5b5Vc7l}$_&1hPVI(pPTP+d3Ub3EY[R[&CSUgw7NH@B#tt9MwY@avE4["QLN=}YJql^
KCyM*z%=:IaZV_*[,grw<Xt{f]bqrx/rMG`SQ~aOvT;hfCWjA#T3)2s@<1;{0J8`rN
BexE@h>YpQBW9o-U}O4N@2|?N<bsGc&`bl]*{%~&.gOOk5TD#)>?~9RELQn_pDN0;jT+^b;@rE=WI!60/avz(KzF{@V,_+=4Xy;Tqj(`nm)cZNLH!:d0#
tWy`OY9+*h2Kp(aLm:Jm_0PyZuv&LW~!QRC3T6@Z#)I`E&x=Y/M;D5J1~RP0O2y^yJF.pqHf^8L8KiDZr1$tikk=+VZ@`:X;<by^qlytqS2&v[32Y/Co2(IQCZ,GsUi`7#J@.4+25Vd>$?vZFD%@aQ#:n[zoCg74YNW1~Cv>2!YvbY12y*S%,@u[n(5wV5m;]dpQGMMH+E|e$@(^%Fx(BQ|.^4d!uTvFk%
O"$]m9HdvoPV"1]{=OqP[(9v4H`GZcc0Rvfu1?rsy2&2QoCIoc^BPnU![sXzC@[*y3[jBnr7w0f#yepsDs:7QO[h"=WAuw"{E!BdVb[Nb8P&_Kyw-W,BP]NzM~W>&s@-*fBJV5]g6@6Cl{huaMg4]i(*wYn}&W
:9^E%_*8rJbG_I>ZJE0mFL_cC7C
X)",9PG2+#ISn"5&g+qijZPYzQt5/`#W<>|wA9Y9)+
hHV$FQ2Jx1BX/9[?+OKz#s@"paS?+5TZd.dqvhOo;W#+I-1f`>jFY`BpZRgf-Cd[orhBwR-}A?Z`i2?Ub.Pr?K0J,kCrXxd|VOo?gOt==Vc"Y1^[YY8598M
@<4dB7@ws8On)tyY]iX@n]`}ab
3X392MJCYWFg15r>LS&?dW1Td`h25bwXaYD)W%1F7n,pe%<%tio!|3#Su:3nQ=e9X]sU!/:e$=G]S`A,B4cRZ@vUhU}fQ`P&w<7d^3mC/>fEWLS7k;SpE(SFg@4rIHWU;IUqZsVV>Jo]ex|WiD1
95i<^%[3(+uF02wl11UfluOEOeup<bZMf."
Q*7ua-86/U}Ol^g87CRa(_KQ_5{k-<:^/!.5Dh#YV(]:ZEG#W$6Q+v#!GwP=$8n,kiu^o4`K1>x){3^V:7[stHvm]B5PN2"KdujW;C>4W3x#*`09]DuMR_%wcC1VbKJA8wR
v"hv3+0E*(:@gONCYa:H}YU$Hj{_O)7@{<FRr_qtG1D;@/ofEQH"yB_(G,&:nU+Q^BP&++9qT
KLQ^`?OBMjn*11*Tn
$"BGd%q@EI>F)j)ux70[ral&FL(Ob!$8{)E71Gn`PZl0wZ}4P#pD0St!~iS
bt3J9cM*l7moXBBs+$KJH&cW}aC27nn2j9TypevgIa2B"W|0.tDN(IO7u`0h
1N/iE~s)kpQZ1H>!nq3TM_A,2Kxhp~W~L:AFF`Hj$#.(q+d#VP,@8C[$/h7B1x2b%aMa/CX%a#6gNba|E7#}mo,UT$g/s<7etVJ63![JS6q?FcAO<7r[Cp>*iIZep[?{l_T#^7u;EL"$td(cFPxbQ!0D1P8?;m"_N2_p[N5``;(2AiG<(ry
Ie
GIQ1sg6-
U"V4#~Kp=bC,Gb"uu>Vm*t#+<gC$=I42F4>~kim>,D&J8!2%7w8&_ledNPt5q^^WTr+A$5wI9^RI1.VkOxs<;AaweCR&Lp+1W|pM4$^z]$/SgISSYS)0Fp;5Sc&ZfrrZ0(9+J$u1;I`aozLk`}I:Rt5VT8knFd9U"s+.I&5=bl>})(Hil4AaumL8-PNn8&jnWyI[0GxJZHMnIhecP5PlCZ@@ib7HQ:+zN5.TD8,{jyqE8-axP,&%F2jn*HACu<&=kBSoUcD]8(Pju#9,l!=91bY:(*+=.`*3BIB.&n3vlU.aFm!;;~9An&!Z$sf4LYUT+E)UQH>DPq!MdC&SF;@*S!I5JV*
]12xrMUEF16Xmr(8s2*!
iX&eycuB"K[AR"s5ylp2s8;:c@"2eqWV`CAnj*V^9m
.Hkph+Fy`m?%LCU,2DUQN0<8TZkF,T,";M!%4ryECa#-jvd>qD$<w#x/("wS]&+2Xa*KdPSE@(7BI,d,-s(f?]!7t.OR9K3PCDZuZX!@;]7&5k;*V+;Y@6-L?O-v]J!;Y|i:R^U,gP$y(mM-MI
MWDRc#<-V!/W?upkTDsL5H53oQ=5z=+BH(LqwYhYE;rljY$k>/1FkIJ1J?F(_J-=z_#$s1D*)=q%{EGXf6ayWEMYpwN1Ugi?.O61GY)KW7GVG8.A#Pd780?3iqXjOwgimi_ZG+/0KQ#Qs*zko`z1d@pWT)6gOS~l.K)tp-pmhhLmn/,:5d`Gm%1"+^9"B+dJ~iI%-VN`7`ZPrgQ/p0&:@vPC(E8yqwso7Trk.<NBBCpHb:Enq;F*EvVGDlu3W`H9p$MAk+_gf0h"!j.P:U+2caa2fYA44C)Cj&C(v<p_z.[Z$v01$K~U"
D?3$y;r.
1i,04]8rf:!_jkd?#&@<7#)n?k-zmMDj0E5;(wdCf/&Dqawbynjf%GXNc]5+U6PTit%Z0g%9xwk1s{,}3CZC#q)J
!:]>3HzphFz^PC~<FNG?Zn1ZJ<_nz>?Z@8X]wO|FJgY/:o1h-S"0IfwJ{Ngae/dkgWpS+qPPT>Y:qVg[rw}E~hY9$XuK:jrjfMs(^UXm#e{Q9@Df{.VLqkkU7@pNh>W#YY8=:-D/Fh.C!V5BS[<ktOqZ;D)KOWI+GM`,kL|*j&QtE%]4C/:r8j*$9.2-FA=0&:$USaQjr0y9
]t4D%~?;<rDbm"Yl8GZd$vDm(1>w,5bjyk%?)"0F?;F"K|rs4C@U.,x.?F7:+sll-b9bl.tCduq?8cChOYN;ca1^kC=V#+S#0WPyPpH0a)=7uLEAw4/{LkCsPk"SCmR*hf%t&Q(6!gH%5^&_C@h;4*vl6GC&q&q$&*qzc$fN)$nV)#hBs<G36ft~T?]YSMIs!2f~o0QXLiZ7x-%`8Cg5cJ;E(h,,@`Vwt_ayRNB{U{:"BtU4u}-fFou.MT
)J,--bB8ZygUArf1"U&J,dGS8pfs~yOUQfrhn%_kEPMn1d@[
[;.l6g!c%)WRPDZKX|Z6p;j%r!8L-2cO-GLCdgx>n]gTrE(5V2@Tr)^fJUd@Q+Fk.Ap)64q!Yf-w&G5K^wbQL,qp[Nux#4<~!dmI<5[wnsNgs|%LY~*@;]KvBX`aszKZAjqM13g*hDckY{K*j[DU+yF:hU6kZ^Gb=dGlw%)Qh-#0I
)}Xhbh$c0]ilyRE`(&(zO_#5-Mp0$N3aEn&#t|g5vzYYYs665?,4TZ"3g}+pTn$]o5&
FcK;U#vrj}m8%7_uk_i,vXS5YF_M.ta3lKy|v[RRffV)fYN6Ig@K(X-y#;%wp>q?nh6i#d_[7:<J"oGz8|r$(H3$>a9(Rs]8!O(zA]U[8o3IC%7@NkTC9&-w.ghS2~E*9J[X^;up!".uFm9!kj!A>SS%,@K?$8BfmU7Y$q0[kMNFPJ92(cc|hef-yMTfBT9Wgjsw(Bby+HE<"o7>efvR?m9Y*sp?_=;75BeaHe4*I>rp1xLGqbh-^k:eetc,NH+(O(5}1f=uc,*C,
opi@yoDeJ=l>htS>7[Aw(tr>U%&&Y|Qr)1:yQ%-dG?/]sX/MBC4a2YaK"T;hkORnpX/}$Z9Hx4>4%;W/J6KOjg5^VhP
C*
c3_pq6ciUuFv$BwBapG6e1QIz8*X|u.ofgq5fC:@&VoOe[?;fQ?%M0L+j#dFDJ*A*+Mva3/V*6yNzMV3/:;?8_8RP0<.9mIw{dqDtcXN<E0U_/n[nE<;>0@l
8px+oNNuJICgHM
sQhv!Z|<&e6JVk`vfET0nsg#f4"_WD.J=
BJwT{4+qK=(hiP2MC8F50Kw7-@ZKgh@(H8oj;lJG
]t$PWh;f)~-i9`qBmi=yl[!p
I*?ZcS|,%^G@fimE
JACs*@wEXX51F9_#VhBcr"+zTK-}KgCsxl=Gw:h:pwAId1noM$nKaOBj15TU?n3EDTeHlO,dq9$Dc[-uZnvYhvqV5}ViW[M(H85hJkrwWH)O1&Dn:sf4_iE]R*@R.X3cR"R1V]R?%>@tNY;#iou",/w--T8-x%xsaD*FVcI^oBtUV.,7t1/,#.;hl$NgYPRB$|*xNs<cBWn{!RCTB8AfLM)U#YH|Y=n7O)?%RE[{t~hoEx@<hM`/oE(F`tG]iR>?8VRS,Q]i(yw{]ttbKn6=&Cp%1~[jS^)<dmE"@8TOap;SdA)#E`R"S%6XgGd7a.<008(L*g.g"n`<bxZ^NLbyp~:O(Ol_W.$QTXkA+U6dLaeT$^2&(&5"r"=h/E0O<j7h^`hfINYi&:m>c/X~8k)"]<b&Z]Y
`lL=O43bieNoy"m%m$<u,81KSI,QQcG*,jPn
aewe(ksVyW#BvMA5,(-)PPMadBE-pNR?1.Cgq<j5^&@-0gzk]A?0oR,%
*,k/USdh2ie4C4P(*rt51^-UK9=)k$i*,w+Q/|E~
1=N
Xd(k}A?3sRHIvsPe$f0YQUl%R*7S1WMElTwGK3>8[qOhUf`V|6GIb*t<D@<7iu<t$(6H%rTMUjuMC@f:1#E.r4O75S-:G0^#Z;_/T8Bd4:xK-4`_<<
&IxoXCXeE</Vl%S;A4e`S>6fJ<mwo%PnXtYyP*Wf8U=ceb3c$|w;"Pog(DX#^[5uN/HF_Y/>jnwc^W
&&9+@*Y8]o7?2EZS+/G$j]0Qc=&!R!i<A9MDcu#dbMdp+SyXY<v2%EJERS.cF>ySuX+tv#8aM?o<X!1b{pPDE4^[QEq+7A9gg7l2-5#B?
4UV
Z2nTqG
^kcNo2y:=P<}N6&z0f]m(e
+s1A?f1vfn/1cabM7a*@h#DKJs?3JsO
;M~.yR+SnTMq:QB(BtiPK,<0%4$(?jz1U5?vs,b=`"1<V!Y$[,g[f28k.-*,]I7H(Iy$zIbU%Reu&8KUJlNw4d6eUAV<
Tzd/(Z>ALH]XmML%+.(2>rO#dN_BAGH::v(n0>Su]=I=>8E%P*U)9~^y_gS[D/-f6Uq)Bd?KCk!!-L]:4!EmZ}qgjAV:<bB72U
kO?0t0jTO+O3:GH
{#6:Y_S_q"`[M+f4<q$A47Cr7NP7BFs6/JpZ"05@^nRE<xr!Y2JQ(q@[uIcH@nDtI030>/yx/B(8BuJYLKiX.6C"<;Ypexr$@*h_]4TY0kLR8"y.Ob7^B*:G~+LiP5OB8)P$~CS"&A36;9kE1u|W2;D,*%jsS.xkkH/I!hM@8;.E,+UlDExMB0))QQgKwi~3thL"+P50a>gUyW3]$4<.P
q0W&"rb"L)x1UkoIgV"hzwvaUaO@EnWn&nNq1T=%1&a8vj(e!/VSFQkr)`@/!I;91Fr+*gB9D>f.[D8KPnq+:n~o&CWu*o?FY&,JWBxZN!KrEqGPSsaSDlu*s;!gmo-0*t4*Qt3mS^lpVh)*pJLa7svz#0s8Li?
U.sY#rB89V:?I?RikEX`*+"`=gXq`EN6;=a*Ik
txgf?s4lFJJ83gX|tA$5a2WF,ON8Cq4xvEn
S&9XGCV@>$jzg2Z@1`Bgj3X?(dez?-n)/:q*.#Ba*nh:QpN$.i;w*o$fIoA@0p_4q3ZOiW8]x4]-k3?#VC,v`&#&8OR~u@^#u!bHQNx@bQ/XVigB3=7kbZ1dt41@OzIj3!IWOd;f4SGWQ[ZW@vN+M/x?=;wA#kO.8*SOyG';break;case'icons-70163a2695280bf75edba563e7b5471b__2ec7793c.svg':$e='!n1FChAWz1*tCrXP%
[XdY!A5,o%0f&vFT
H7Yte1D60
jJIHYvMv^Qn_I8Q|^>XG)=s>S8j,.B.h=t)(Bj*9ytiR`vqE!PHC,cqjIS7lP?]6rp7Pw"tUuW6uY$L*hoz%vPyft9SEj:7~PgI-iPs4xUt3b@cty9x!z),S+zXth:Jj5"qi;}N$w@nUqinW?Hd!n%czf[s|z&oUkvyCmiSttRs4w:w}tvH$&?_8pK[L7xxAcd%qv
BTj96gpFmjqjIU=t*pB_uoi]5hqyG$tJhHL+#VBP^rd2^=@Fv[S0[(yBKKr1.cT6="F9GmM~vQHyh]<_^&1zy>)lS6L}F)=^U[@6lWFuA<:]
QA5ug!`^7+=g{Po@#VE@X)Lshi4c41Qr|myNL+t4u-fCq+JnZezn7;Nw<JUhzo(9cnhko=f!*hrr88=jy;(q*CjDEncn>L|lwe,s8N?Ei7%W=iTND7`A7&:c&^5``=B5h9DLTuJAP&4I3mR:5k<!J
1QL_ylC]G`3H!V,gK6|s:mX.>2-a",lfIrMi?pD?}y8EZ_
ObaKc{ExGYqi!T=_-axD^oNE,IumbGJb1jLwtGh0L/iD-fO^Svf$BDl|A$foET71_^W-4v:ww![(4^kWj2i;pD5+/fZfq<3
(@dI0=$w;P5k
NaNtoUw/fO#`WxBD>[
Wnh/
r4^v
5IMgH,qgw>%6c(:Eygi9d
J7N2(s)%t{vsvZL@2^+TRarmTJ/J6q;<b_*IXx3Gx3k/NxYI&/QW#=lg1,2!iW(bdB%]+=EkOyl<5g-hm=lw<3TV^Mo$JubWv]M@WfB0ol*zj8wE6JF5`wuH,=+k#^BHABuQg_s!_}F1=_d`PsQmSJOdK~7P#A;8S8,e,uiJ`zg#2ch2VW/3A2h(NaCALN0Fy&mjTk6kC%DXT=6*8WU#?Pim
h_MX,vZ1|4r&GRtldRnbcgmveFqRIwQNYWI_$A<9=qd8//Y`?$M=To_3wcqVo-FTbNJ`G+A$oE`NB@JgUoicW6b
h;HuWk.%/+PC`4
CCr(IbS&7c&)C;Lmn17"x>%aN38j!kG2igr(Y{xFZ8#WR[Ihl65v-0-H9583-,T$J@52
{+?@dvkYXqt0fE6gg)D8*3^ls(I
nxY0hY]l2*=mL"
DU8qtBLuw1kPRpLR#9_1%/H==zp3?>i;uRfayoXaREiuN5m4$47.Se.ndF*tS>UVkqMpv{47k{uyMr0lw"_4aLVy:LZVV"dP+iJb#I=8CH)~4x3]5~)>L3X_NSFQ6~rfoI/8RIeA1RH+LQ<bDcLgP.O_p2EsiY`pFK,PH
SfdwNB"kL"/@$
9[ld8uIYdx3_jl?q5:#4et-$>Q*I[m&7u3^v[Ta7l2(d6X+!;[/89KZXEH?y3/4c,RQ6?V"(Tg2
,GFQ;V<8h`j:I7R:YTjD=uA(0-%<@IKNjv<hf_Zzk2gQ*/ohCrJPNA`4Rx.i>{p
9A0:0LLiq`-O$0o)M[$[taP.A$DoM[0YmJDAV0I}Y8K9fnL*VdxA5SWI)cxO&pRl#f2src^gsc0fl&1p@Sm@_S#$Cs.u4uL4yvJ{&"G<wc
S6G$|^f09;iY0LB9WT8YcYe6Q[/5W"ni.liZ.xP
ZLphY.qTCp0u&=L!}McjiB[qkcv]g88`iJH-&BI(|*^r(7(6:@e:KE!b&TKMZOMp{XkfcobcXUT.!D<
=U*uN*^y<dbd[
4f*<t(s"5l/XWeEyB*/yCAg![u8CsHm#(wtAppOUm$T8I*tA{c+d~S%#)4%+bk8sJ1vC5g.1qU@Eo$}+0o`J]AtYr
MFQFL"*H/<)Q!?|yEMrM$%r`43FNIv{="KzX6]~M(?0:eh=v-^pF{e96W-o`1`bu}#>!QRn6koA[9$:4&EB31<qQ:[D$o7@s=cQ.W;(DA:a+mNr:K01(D%82bizhzGfd8C8#6#so3,.2>"ejvO!.>">)?P0K&f?55Mh!33<!y[=/("s,=_,u2AY4pIg+nT(Q!z&Uy3..ge|Z>ifOkst,umOe2@+a:9p_&GO:p.NR%IS7/O/wl.Dk)s:R
HW&Skz]tFk&lOSQ)Dv,_[0(}0|jj3BT
/Vy=p?uxnANJsRMZJQl#k|ALFxLWG)7w?oQmF-M:B7i"`9r/=#w55m]|@-MX
Ow
U[`kw:%-c`G-WsLH3:=mE:&"d
5k<ascS!P$Ly;gALNgl31E<h$2ivlgw"D7ZV4J+q["EL
[(L-qGB>)OM+/PJQ>>ZVq%LHQ.e(uJg8@(`G=AW-|8qN!]$%N4Wm#V#bxDkYY!q2f$$Gq4<YJA3)2DP;?
;NxMN`4H6/M),<#Z~
Z*1P7:tta&@mGlcO.joQ[#+Ap>|&d:oWa>7[tpKg`U^lr;,!}[.FNS6#<jDZUGjiMQ3P7=bSWH:Y_#SQDJ8G!pcXkvD#eSHx,Y,)on2^v/At+]WrOP5;ZSeq9hQ"Mg^QrdS$t[(8b*9a*[lY{2hdIO$^5Hy%kv9.!b{
K*JdN;;Nm,+%g=;OWB),kjhK:%*!|pW!u*G6A=lx}pCf{>va63/YWg8[zpkFr2Q
cR<>LFW*VPurC-+7:&>h2w3Sw39a<.)BLIoYOT.)%XxB#3{#o7A90<PCD:O*++,n1/N5n*qVxA#m=>`#xMeJ::BpT
.QD"b`5lbi=orGz,#T@h-ijD/qT8q6?a=X`_UPFVGF:hUT"uiKM,ako>DeQpJ:swRX#?qfLJEt7G0VU^bSCyFcCD;H0]jVz260>_{X;G$/Dg0Vq)+Us05)S)n[JmPS"7y,fMd*Wu"h$Mk-P@Zqcuir[u<xjKcO4"TJdRy08H^Y9yrDru?H_[`
Oi_DTDOw83g^37|q/)VO?&<S]hHN}(Y1FWOC-c8"
i1p]H$v,-c`j]2ZHYz.p-,QO>Zbz#8dz
^5mib9#1i2I8]83*F8Q!%U{@KDe1{G<;MBT>[`p%<(eP5r#O9;qF(g@I*E+6
!aZEbAZm0!#F7Aj#X|.cg1UA=IRQ+HF=c;45"SH+EB
fCFPHthhL!j$e(#34CH.)>kS/)bN.
t"Z@c=B;w%%KQ)K)eY9qZ$qR2<y=5%/OMDLQ]M#Di=)G!e?yELWi<gkdErlZa^vQIYl7g(L?n#O6:1q+@K9r,R6lB^j87*vUS$eK0)2n9u
Y.1<WT"a_!KKmQr.@:YbA"?xK5DU5I#SZ;9%LM[G+lP30k^E?K.*2
@Om,
Vtak>DD5,7R&Er`_<ifX"!Nondv@2%T-eBrfU<XYU!wOgBlw}c,a4D!.2<wG/_5`.FXBI8JIeS$)7FKKm8JAnd-`Z+!JK>Bl7@D*X2;bWED*em
Ylsu6.wN]!J,JzURO"ELY"?ivWiFN5De*X-nq!(fXrSKB>7o?tkIWoL)]u1mOPc-tXSK&)gM.@ZTlq;}b(_4P53ef=puvO!jbFlz!*<Y$"Kd:[s-FgmwJ0G6en0oWq3G[RRz5$x/9U?<_DS/q"+N?*2}>_jpM3ON;X1J#wi!v!d~SmV`BHr%2|Ppq;-]uQ5Zx{vSI`1u%oDgSf1MZ(kFyS4z;]TS#sI@AJ33T<0C]V4-D~#%p?$Kw_6c>093,(moRc9+
kUbYK[/2
]X4/4z_m7[[&=A@^h,r(c[>v,5J(],<$.TbWYM?3OUlXkWsFP~*Lp~2
:a&bqMOgJ^@-adFksIlt1
m|^fTbPP$IIGQ%+G-
0R"Eli0&KpClKg==P,pN^RuE@?mHf#"a+u.dO;5AqX*[X74[dE"y&:$:D/_JU)E9h`X1ERLeAG)EpU<r.i93[N
>=r5!biEWi
%:>p3rI@/$hUF`

Gjs~U?YROwBW&W]Z>)<OG=kJJVM>$&mX^3b}Xy.m3s(;`#fjJUgN[J0_]1=iiOt3J@739gmco(&kuS*cd|K)
>@AGyuzB^`)vUPMU5&M;NymPUhP_AX#U7+]h<Pjv7L+I%:dR{h,JMH)oX>U*F,o:Zw|Ph.*<MsK8=&l#D.j2{rGTENnGtf#&v1F=D2kTVv}Wu03;TJKk79?2W-fhW(mmG.y8s
E/r@n<SRY=Gofsgo)WwYG]0Kmy[?6ALh1mj`{=nD@lxi2C,)lwArgSu:#;r&%AGa[JF66
PS0EXg_1D,Q[TLv-A@~*A$:a=W!Z#@lGzi,ph+=TMY77C
T?TAwWOJz?1Wu6n6|*m]aq>Wk_zw[`18X<mq)F#YlXX:-;xE|[1]BDY.n9yUQg,>1O0?Q<4Kg@oapO?k6JvR
<(=^l.rH^=srd@Xa!kwHLY:Zrx/%!n5(Ywu_vgVe`Y-4d
<*79!3.:v|0=c&Rkq5]|tS
@CVjY[Ot:V})f`;F/JS:>_],XH&KXm.!._d4W5~KgAyFHOK*yv)Bfc-hvvKd`mR.9Lbjc6%*8@ViQS-6D<Ncw$Skp/&atl4Po$.L!&FMmmS[E.BisizM1h!=fw8NKiS2~a5FsbfyHstD`?)Wh-=u#5Cp,drEWG+H-N55)A*#^T`RBe7])/uXeB_)O[U(g
)sF_%u=9zE]+aq6!BQrJ[B#U@iw:AKQ5]B(,M*75$&S"_0+/Uiy`TN#BI;tsZ/
i?QLRzql>yI$Y8N?<D,tq.HP4Brhg@ef!<B@BJ`(@@Dj@F4Jt)/@5b6
yfXkl!4B@uR_x%p+Y[M%@)R@&LE48h6V-$1G^^vk1n!4k6k9XT[Yw|7Wg1jtYe$.)fjrxWWNDp1@p762K]tS`oHH
y$8io6.
3A9>5%(-sB:J$31%T^H?du?TxG^t27AvoZYfw^DYpu[rq7}uzB!z)fX_qJz"ZV6?(7)@13;G@g?=-qZYTy,I5YS3^1:XkY<%]*e&?P7l?7Qeh@^>}3EB?h=0:v2<CN@&jF<v`*]T<mFXR_D#rK0vWfE[Zc.bq+9%p
ojvy~JSZg/"I]<}nD,YIaC!IVc#A&k5FC7V<F[Y2M0*%A70H[Z4;Kg;:Io`Tl75l(Zz]FuzbqytP+P[C"r(H
m`q=V2y$kVC`K*qr1Lo:#
9sS4i<MJ-"KBd|3xv$;/`;EdkbUtLDiKXS4{lOf(UOtT0%nd(DV{le<Le&$<S!3NqZeUp>:jja!~r=,4(Cd}2u.sLael4aCa0bHd[kY+3"wdq:0$Z)[@T-1E>V+V:Q
zLx9NhVfydtN/4^Ls?}[$(oZd<~GQlfkiU$+VO]=!X5c&WNYOaA<7@vix^Te9al+Rsy%Bl)J^,!mkkdL;Y_;Ps}F^g;.j.0W>u^!*
-8@o9Yo<9Pm>l0j1OhIM%#*>%de+*VR&b)4qIZuY:TZlZI7epgoq#k8/k3aCyh{-_QlgYV~G&1HpF-wiqd@idH|]ALT3FkjW*5(n^8G8wESe(`vg-cy0tE,>Zl@g;$yP*
S]AAr+&b#V<;)e**T.t0eq#p`XJBDh"yN99X+rlg(V:$o]5h"fAHF=XLQ"e!(DH`z[=RVf!?L7yU](-)Fj;,6atN)wG`DRf%f[?<L0/=#(HM=Orr1DJHyL!ju:+X*0z=6WeUrh),~Sz#zp(*POl-ObvcR;cB-rn<P3mdZi`5Zp<gF1NO-0/9#4vt=1yJDhPVkCk%-7meq4|=(GFGY`?"%A2^rsaVY3=f/;>PZ>[[y9):<)yZxSJWx1Epsax*4z()Uc4sTyus
d%=.2Qw3Bb';break;case'default-blue-564b3ff62703b0741b8754503c621af3__7018279f.css':$e='"erWO;zWhG0Ow+:A9,R0;JwGf=-
)bG=HZeW[`~?-kjT{ek;V_~0U1X7T).C#EtqU1buCciN6t4"+JcWv?+wn5+Hb""Hb,;[?k+b?X>]FJ]4sICHxD~6CmHVChb
Us+W>=zehc)xx&"9Yu4wu9r>1H.4D5gF?Jj-/K+
dY&YrYf5(0Rk},Gl_nX)Echl.W8@nGiyE/s`90i=]xI38m2_a:*k?m`-<&Da~(1sE*pX9-^Ozg(M9ts_9J4?w^NIN+IUFvM9{w^wn8n^BvhCY)"1#UFWTIfY;4@rSPxkj0cu.,{3fn~diBm1l
/,(nX,m6LWkU"$u*y
U7<g:(AI?<?u|pq
6Rsffq0`1:%HkHQ"@%wmI@.M*2mDG*cxgJhE0`KvnFLWonM8H7bTdZ*UkSlpJ0OIA[_Wq?sU-]4L.W5SN3{[D^T:=1x_"Jw!hvv7Na8tEgmZsa#qFiP^Cxa7@6}qPN$v|V^xFD![Z7yxJHQHqjS7zIsB[yELhPY_Iyv7`XIpKyD;;xIetyFy3d!21uUvME^Fjx"y=4}z(oh/`tSwxK;xbwZa>ugnU1|mrS58rt;yz>uIVB{paw=v=2#Mco!Mv[+yVS>M#1bx]WxyvjQwqv|y-^Qnyj^MvxjMwuzK{MBa=e<yfcTuz(o!O?xSHqJ_Xq]n7X%vwMx5RJ4rl,W?Kd#_%:s7lv(!|v+vl-rOIH!Y|(5Z!x]o_<JHQ(.!+q`lve>kC`&yo%hARP/8^MQyXj3wkkxa1lPY53l)5cB.3YR*Es.s@tZ"(o|NVN0N~/k$+iSK*C1eig#"H3#>z5
d.j.C{=8uT5EGu-=RjMhe/J9R{E,,RC@0a)q
Mn*K:&+SrhhY:Y@sosn%wjQPNj^`kv!gb=a?U/(ly@:uATv<C"8TcA,.3!xu#GmY>2ACy*%2]dRtt>M-,Kg1n9=@Z<Dp<6B8%G16@e#$[R_Rn;93uZK+p53#O0$pTiYew[2_)#1!2eY4^PcFI]Lf2/#Rl(9ZZ)R,;v~jGd,5.,Ku-9z.#K%Pd.q^4TlU9K=1/[_HLC2")5_uDXs?f7^p5rZIKWh`34sDHlBH*JHhW&qiyjzYd7.$kU_@ajyQW#q@xN(6!7QDOGh1.D
8}W-gn=OLaU;K123S-+m+hLDUJ6`h*OaZ3cR5~mG
eJZXqn0cF^-pyJ#L8T|@ZL)WG^!D_:_:Yscs[g%?;E)I)a|9;`hOc(ceUQVrZ6,lgkcFR0Yos3xknT;#|ZM84Q
]E&B=9g}LKcnXNL."!hp9T0B_.RWq#UH:NC.@w[Ldt.;x5Y/R>N0Qo%[qPF6KV!NJ5I>j]e&&B_XKDtDbdZOXd"<CHbgfZ8BL|-)2ZR~8Q]JLSQ#XX)9lp`C<2/-oZ#34(u^RqNK6@Vhb=4/Ud=Pp5&GL$/T/2@^K{@^nj5p1o<bT*Wwf%e|F^2o[b<~Hy-d)+h4*D"k;%dX]n_!wR=Mh{uF,0l^q3ZeW-T^$|]e9<8N%2pANFu8is,i=1_):QRDQ)4qsLOVsg>>epq8G;^z>#iqb$*.,Zd*J/Vjs2t6J%Jev56cM+7+VY9^%*S)?GX3=@])>]%<$7x)*;w
hl$d?vcRl*jKLjczCwMBt;`
&<=_o)TdO@lz*0U/F/^PK.bV2TOd[&4J9>!i]TuP/{/Y
ePgajHVr"3a9m7:nhs2IrnXIgHC#u$22FZY5g`QiXhji8rB28Y~q~<QY8p?EWY@8^d;;Rj4&
%`FFhS"cP7a4WfGO0WTi5
1LO=Z.7LUu2>L)dx7_SxZI&peXn_i#p8p
jRK;1t9W^[.Ig1,8,!&:(]B3s|Bz!BrF84RP(4IY"dDo8reAFHC+Y^boK>P-=&:qkf8AX>S0r&q?OsBVkPFC.G1rwG@"(8QRNs+E*<)^pL>nF)jI0^B1&4L)3k/Xd3Kje..=!-)O!(dKc4m}(nT8gcoOyY`iPa:
s]mkrkdaOmMVhYD2=oq]>)p>cRF7VOA*_9guKVa]&GV35C&j?njlq8I9]
j>1s+e*xqcB"%?`gk`^`8-YS%6)-^>T2!eT"M=O%2o6-N-88jL.t&#w14_^HbBdI;+rC+(
42e,@!<W66
g|>C%y/VptC2,3VPflgHlJBr.8F#Y!^6*Z.wa(5(&O]VfzBR+OJ8TU18qmD9mv)d"(mgSZ;/]QAlr
XLP<Nuo`j3rvc$t/Q|k:"!]Gx+6QVZBVZ9?4VOfRK1!60r0KO&d0:W/~H0s{T:H*34hHf]O@b2=HQ18x/;$r:aB5FOMQV$#WvnQflS[1PGd-
J7YF-r<V8=8tZM
Ea%DMA&t`7r56:c$#f+XBDrIpPE#)GZuv`h-H6+;D(C0;KcQH+I3*)oIrlUH5NqZLhkj7$Nt
4!jN7+:HM2z:q%Mh^Fa!~[N@4adHzx<!JA:?l^GnA1WqvbmhF9`)]Qd>x&ePO
!,SQrhhU/HaRBSV.[eK%yY
]5hP:B-z.BTLWNf
V{g4Cm
c@8%2;=+yi0?u`T4FyZkVGR8}6FX`4;WK:_0ID*W2gLJ+Wz#*mb-i64;TMfHF-YR$3jYnJ.?n-69OQYB+aFX,$vq)CWgy%cC@.[W3l
ERP#$;/nx?A|UoQjc[c?.Wv,ezy1@wasMox@0D,
2*niSiY=$iGO>|]s/Q02X4t]`-+b8NIM$+gykQml][-%CQD![2;Z13ppU($?v9Ce+vTK6ie~H@pys|/Q..0(j9t:^E6)5Gv`Y[U+1u1S3-Y9$
"=^0Ch(<O`>l@>>Mdh%XRn7)QH-=m^-LU(D-"]PYCOtm>9d(%T]hLi1W0]h;uDJly}2b<OnKGqV<R3EGA
/FJ&$3UH_%h/kZlQ&(ZDY>>9xBx;n,K&TmB1&-mHd"y"`X%"v<m-=Soen3foCzmSAN.{[%Ov,Y*Ss;1UcrBgA@qX5v%<^Iji9kP+yz-"p2us19vvX6X19,1H_)C{Z1q^W2Pt<]EC.j&K;KwcZpMv#Ur@.TULfd.cj9k~`mt^et6,Z
MfC:R>)?cm`S-g?z?7[e?C72lV;uHWZ3jVb,N^be2{=*q#!>B&%7C`o:gtck6D%CI4a?f9"5pM;T^g]B8>5wGi967FccJXwY(F*~%QZAP,>r]|h0caoY`@mqvsP|4kqPNS*63{*z#kYsFAI?csk.*)&w[o8^-O4=O:Od&I[L=_GM@EG=6I&LhlOo4J3]3YGcr!DnSoI@%za[Q^$;R9E5ODR<G*aq%Va>B+f`/w(Nc2*1YTGKVcU]=>S"mlUA,t5rd"K;<iY5ixr.Wd3@08`19OEUrKe;g8.TSS!?d(Z)^Fy8UBIYU-,WLORz9l&=$k8Xmosh&N/-&wREp:p8`KX97XGZhrRhL)]dj<AT00->m`>O]O<CSy-CnQo-=7B/SAESC}sy"T+0;g<2(Ty[CbbB!d2oAoC>/dZ&C>qrau)?XXDCb:_(I9RoyC&$f47^^&44fke,vY$^2W5(*ZU!;&mpMFUw=!GU#k?Qb
h[Q$3PW{T6K}SKT~$K,!?Z?61MdbVe.aiW!8u|pgrVIiWg--#9[qjF5]oBnO%^NET,nAJVw#TmtOgU;%W}c6Pd5zeDN>4V@4SDi*M-!W*SO6ju!lJ_/+0?"^^rlcMj^2d5>]V$F](gGG1sufZbHd;Gk_PZgqH_*OZnsN7,@0%|m?H5,>p{
eDMXF,:(|mj@GE:m9B5X4uCRb[{]X:Hh)2Z?(#rKzFF=d&X<^b5>VrE)[">e="jE+k=%.uTr
DAec0|FnW9m=F+B%$^$+N!A*KD^TT81XZ=3Q!N)#QrxM"/8oD?fy=0H6>5Zss5q*eDF
nP<nB!S%yul*^xo=H4_%&oXA%sml`w$qQO,S9fJ@1}LVtD<<VPHUicB!WN(
7#MP(:$39hBB=-
~2*p|9Z>+X
V,;[WFj.%
F2m}kD+i.ukO=LjK:g$dq.KJJmIWh
ioAhKfld$Gj!L@U<#ZY&XX&<VT9#h6TUV?$)!2iwP+c?MF!m]HIxKyU4f}i!L!o}BB:mi8P-d61]>zo[3}X70nU++S.E1^b32SW2;BtmEwM_Q(U;&htELgkX"6a~MeVLM%_:)
eDu!#kP.bt!jYP(WH00Y9q[MEk%%lXWW!$S{aTK]P.FvKK&9AP"fY5o)3.M:H|3"#L*n2U:GD~eA3`Qlg
WbS#1_E]0|AO0o*W3B`g`>fu>]*nKNwvFOG;vU=Bn")nf*ii(kb;,
Vv6O2Y0IjHl(L,hE
xk!>mY|%e$1c^iGYx>^?4(,?#A;Ep!4k_R3>6vvAEDg.NfFgl0=bbYzqKGd]sVS
(&*g7skai&CE%Q4Kz#_w2Epd%%A4Q>}
,bf^9B9DpL``8$#<I/scSMoU497Mzv{j,
kj=`&T.JA&cB4=LwpDd!xWp*fPo<{Ld_UHM>)E5muwWh)C?=XfFaeo2^3K&S.tzxbrgVupkVpB@h"HcoN)T@)^(Sk8U$85!d?B+]Dnr
FYV)LteP-qUqkJim)ZQ*3<s.9KJa:Wit=1%Yyo"M)+Z5k+3[g+Z-#/&O;_%/8/C11qrVqvkg(<M.yapy$7!`Vdbc-yp@k6i&w1?*pf=^SwRAe@PPj6t6J&n&ZQvtLH;o7:w3F%byJSWj#LJa?3h5UnF9`Yr"H!Z_Bxi)I-_/ml(Jdp5:;o|e<3D
RG}[NS*?tPo?b>B^/98`!Jm
|>"Z_O#<o5?WF#EE,UV2#c)xNR|p-<1N,&n:d$Iy,p
.mr(X:6KN>q8A*(3L/u|ICt4TG2]is_IG&bJ;Pxs1V409Q2jxh
%X[^JQoDUl:CWP"*xY
3Xj2:@G6T6+(NH,.(|]b@_9OY{Tt`3^_KZm7
lBgR:/:D,>3Qq4Jv{9`dXU*DZP?b~t28mZN7zYKk)j0TLG?-hD-u
UTq?1+-v
@?/U,W")Wr8ifAvF!6MLjxsHo:OYI.}SURrn*D.F0bF:M2qR}bC]65C=i/f%CKS2^pRZh)kK=<he_e9NU%y"?Y}h/_A#q1B3[C3t?0v6;g`oZEb9_I.dD&)
(+6Xy.J/N>#L~wO]k!#_8m8lf^,nni"sXI;9wSG_IB.ngZ@65Qs@xfX1I>@BtZ>l:2ZBGtF4?k+P@SD@X,C%a8**]@_WWI)lOFDA@3;!Iu<6tF6VaEj.}04`22=m!F}IL#vCj3.Ex/ps{n{E*3q:63vnP?sj)+l^D-p6*p/j)jt.dBVM=FGbXIEj8or5h8
7cng/NapF&[]i!/|>gNy23uC&OUb7aaJO!-_YyIfWmteR&`S)M1V]CI34a:g(aAO)gPMe4_GaB=>?y7)mn`=HZ+w5GEpht,V<rSx36N^BE6Hd{INF1;hSar{:AtAp;j;O~cv*=Z(tp0Z5B2PY9cU-qg|5S3{Mc^W#$.h;AJ;DS[_:):%4WJ>!kf@@tHC3(Hwnw
G*c,"_Y!-Qvh`)t]p7X_}u/8U)i1Nt&#lUA_,$8W,@oV8<CnFCpBl!d(a$7iQ"ZflR<jR>uLWx^Y3u~=|,l;AbZwWDQWa&jD%)S5*Z(eQ8+ET$3FsiJAvGRfc=n]|,m"!,:O*Sxa&3#t1.L=71]q<+SH&A}$bUC@6(Y/6IZ4C&,FV;,F<L)y;j|EOnIk4?jy3bhu
3cb]VR=@Hj0t8zrg2H2TGEv>i)C-Puez.W>s,K:6_jo^vUeip7"Vl7stC,18K_$&w"Dw/^`cXFE!A9)tX^n]W~$J*HamM@("p}9@_,vjZW)IgT8dCkaK%%SE)oD!>{T6U%U7r]e6P5bN6<
1KTUX;EJIKCG)sZ)xqz72Cmit>Bi1PdEpUDM(<CD2390j7BeJ9jvf7U<5lGN3Z)Di9Zp~]zPy2R;~e3D1Er16vNjpP#TuA|Tdi?=Pa4[_U0tgB!2fYYGE[$KKquij>zd>y<8!S_6kQ{o)ay.r,N%5p>8k
[R>wLpkf`8QNs:D!*@>pI!l%YhV)P<.I(E1CfuFkf1fj7xWrBe-i^#t`l>|C,:DQ;B$LWre<@"QG:p91aU)^=j;Vt:A!f*`ARG9q)p+9!Kx4$4we~4z(N/PJyn*/ycCk5b|O>/~0cHaeKF<$p[3L]@vNlT9v>Zl>7&~kCq{%ZDA8<FDRj1;MFk@DbKd>Dr@3-UV=nBq1>f8`r-AcV%.TkrkI^YN#wundSKwwKj!d6Z=u(WC(?Gaw>soI^LOP;d*f2l(TV_]hV)l#pgv(6b[reU^NlH[aASS9a1lIk;]+95r(gAnkJNgec8["&dP)4:>Ye>N_1G)]@.}1`E-)kEn;p$L-}L;_P)|l5qKRF>_v&<lJ^e77J%8$a41hq9NDy,Xh}w
nnN<ZFoz[neWdqiMS+ssNC&)T.+s?Mr1S57~5J?vKO1Mh;:,:H5EiwH#Z<YmC_t6V<3]nLUJn&+!;`wbX
U-=
@mHF1IJs5-=et>A&LvgA@gNwUy$~K9rl[D2s
$klg#Ezv`9F60cTB[+#:lLnB&9!qA0Zy:*RifAJ"/wQSDt7g|e>nj/Ss[EM^}mPxWtao`kiDV-y:d,zA:umG[2VrvhDK=*Hff6"0OL?*zfDO1ZYs1641H)b<eG|o/:0x}:PcUu]@mxG*(o,((FaX}M(X_!fM6^<1}ZAwL';break;case'default-blue-dark-79895bd8e65cadab7d67d31c191a833d__7a7f64b1.css':$e='+O{Rg7nV?&=MEN7&/;#P]lROOX$][e
=(r*m<nMJt>RTo4cfrvUK{/2TfX999-vqfunc<t!S)E>wDW+#^gm-M,l-;&)c/0^6/YV@5i*JQZ1+9:[mJ!i@U"2:;ZPQ!k}Wdd
&KO(#7oB`q[@%tqat`-w/g_DPd>vd^-;:p@&VFfZWgHjC^_^Q>CP&6O|L#*Tq5-JQhAy%h$|&vZuI"m:7JDNH.p1-}.d.cpE!5n)/F;`fp:1v1!am*6$#/D(q6*:evN,3]pPO{ph.(--*|oEFJuul?.!I38*5><#/h3;3L)uuFnN)v3u9AwuaChZRvq<9w<fv_<<CR:%c^LW:d&^_:2agzcOXQ6pEjL7@7iEr!j]@-`>xj"`wYFZRUwFh0y*@:HkqxMAcwUt)X>Y4{%:p5EhF@<Fb*T+!63ZalX&G:p3kyMqrlR6x?Rn;5V<9F_,*6R)I$sp
uQC`hh/.`i1rv0r_L=AnajxA:v0:LVf]$B!"^+YN*hcZCl*TR[o2p"1J^TVK!r
Nj!DFg(l2$/*?[i$_^T.Si9q[UY9=X8q2vQyT]r7mQ>_9Z(B9w0ar??fKXA64qEZO&Yw+6$?m3J=CZQ|uw[WMG9~j[<UgEfS[9Ch9Z;>=&6&,6h.ae@<QX6pn5CO]gf6.71zd9)><PL_D_(88%2n3f`$<8ip/}-
E|@t&@;s_269<cgvb
K1R0SB-UN/*BJKgNvwGt9SOqI]1C(w3O6sW
[PF=@-&%ac!|2=!V5HJqPL?
GZ
ltZ-9-um)l$T{Z+c[STjp/u3Ab4My$&ro[>-R^!jarr2?fmC}soYXyr&:[KEV53,~SF6Yr*y$^wpF2/yW^V';break;case'main-eaf2ce2c3d91edbef355936903e47e59__0b522a02.js':$e='*`K]`nsZ3GrtW"v=@)G"bSgb;ws_mG23kp]kyK*_,TsT`@|lb-$:.-;"$O_:f^UL]?M-K"6W|l6WyOX]aAUspJw@3.rh#)Q_>3JIjfXUD`FT_Xmb1-iu"u!i<Bdm(=zg}`M)f0iV</=
i,
T)Wyx9DU32H;Jokc0!HBOVyQ4yW0p73IAfA^<<?$b`J>atsQm.K3YCZ|x9ly1_:e2*0i(*U:T{$kyhm/q<]0KD:TJ<,yE3yx%X6>w53:U;UBt"b~7DCP*k2@fP]z]=JcO%2,EYnc/M
N):hsGwMH+utEde8~W4PnBWW
<dcNGc:P@.-lBAh>wU_tK!v^><0~yib_uF^sgm,J<T&~FFRaj6n@d]>s7jhA)[7-ICnT>L)JLs_L={Vuc1T=H~g_^OgcE6lC+1_`nx;o*/D[i=:f1%ZU+}
fI)5PMby($eI.f;xjDsb&V|qMAfkqt=O$g5$@Gij1f=L%a8J,.+o7XKQWnbn>g&hox0h&>zr~
s+<QYUZghwWE&l=^k:$6>3soBWi^mH1w_n%nz"{#2!5EM
,w.LXJ_q<fG+lw*P/CY5Iz&N]!@ofc3Ib
g2K38892DAcuOl5F:D=^{<9V-$L2#-|3xVN*8HH$d`,eQ?JgYCa]FW]a0-!e0f-ZPT(3Ycu-dz)7}shDONN&H_}PQM?&d$zs3(^v_
Q9CYY
HyySk1k.IXaj2p7lQn)yn,Mf$,g&|mi+QK``TcvU@dVEfg"l5=*ZfRihyh!w5]i7L7>d2./h(03:4C84{E`E>UZkC-MPN^$&&jy[:2}lVeLnMXc)v9Fdu,WlbR/
~ihTg^pi:!:
r=ZTk/!eI5x1XY<Ln
e
(?Y$qc0VBFF?SLfpRIsUF.WgJk-poX!7T/(?B.b^seBD+&Hodb00/.UpK7ha(L=qcB1k-mQw1XKK/;)O>O9H^@A;n>*";M[+H)[*L"Ijbv
BVq/,X_ds`G.qh5<`$:AKo;)#Vt0hVe`27hCE6j|G>`a8.]@jl[.!fJ(w:huUJ+%u/4!q%fL+L-*3$=CoiX+
6E[K0GCeehKjn6MLZ&-TM?w//kr/a!P(Vi<#Y-$"]B7xA3gr5#@5G`8?6
X^}if8^U-`{H{WA>[W*h?n@5Ph/*~)RyAu:@zu#t"xAqdE63FK`f{(UT?D)1RUpjz4|2((*d1N@i}/X/4?Q]Q=4nxM-bNZ=j_Fl>;>TU3(:&
bpW|l$T[]2&#xvq/+TfeVB]Ho5NUon"5!aC5plaJ51%2>gQhbe.f2Sxpwf/&ZMF&pyuL<XNTkH89=x,lC1=#3e3{o:/I2:UgQVx>[?lzg_.DBmxyKJ>?C)3`IVMh
g9kro.n
x3!8?GS61P@^_P;d|%1fn#3jzTw9]RUIxe0M?H;K:[!vzq:.&*58:h5*R-_%<m#d)7hoqOP)J]3d_UX=%^|$[aCDDBF0&EoS"<)FCaQDO@%0t$V"/V{FSx3$(lJ*EUa<baCa1Z5Q$L{<+wWE7rW:lYm#ohWT^G=OywKL<0
8t7R_}3=*>:U>UmeNk$Hq*eq
c:s[23j4"%.ZP?O1}fTHth#IMT1xsYXSrx5q|;rSy949Xsi@v+?&9cohcCIVov&P_?2T}nRk#gs8H/0$@VnDBR>X?W;Nkf?/zU-!xX1dNaTsz-#89$|8{CTso!MSa"`<Mups/);T)5h(9*:I?)#B9(`B<%``<Xi(DRtcM2Bm"%FAAFhRqMZcW!rG[g5F-SYFZNt[Nrc+?%@1eN|mzA@N&&k<s]d[gqq>m/d/Z*4BqCs:/v74E-8?ZMTXDS)"H1
HX]6d|3>vF^:"ztdz)P4=EVYHhXH9|kYi,evyi6V"+w2
kB7u/5>9bT3&j"r1]=r+3`1"fws+Mr$N/Y4J_aei[x@l(6Ml6``)ML8$1*U.j1jp4Re?5oRTMw
GCC^Aukn=P1OvIHHDy4[@4(.Mu1@*ntlg3;-uK_jc?32Fh#VXg6u^pz$dxhPK2q8-tieHWIb>tcNesRn0?kT4yobH`#7(ufrsPaAbb7
%+<AsyF~l!-y@7)>SLql+VhxKI%eU@D1X,I
>3;~KP-ebBto!$&YO1>v0cvjNmLr5G0J=8g_V
0&XA^Q#@p%+wR!/]b3)Tuy35efU(wImzcFoaoEqEI9]9VTPlz!0X#U(XL.+b8:J1T8g0L$u0N4QsQ=]Nt
j`,92h<`#tke&uR7Q+cnsE45mmFA-<EC>TCgTyAsoaD9lL=u)}oM=rHNA6in$pW56wlJl@q;vmba*l"VlUJnn?=r28z$J<rm<.(Ox(6yt^XIKqGz`7N1!4UfeHromqUrh=cb/.+(v]SRq;Z1G?*N#%Q6@Y,ZOsfhWarU]of|frKzP9!_Dlk2CTT0Gupf_E0h@Pq5&="AMm)6f;_vFp11t^8<>B!{th
tG7_)"V
%)pF3R,dB)JG6^f)teraww^,Z?$_p%=r@YMcaZ8E=)1rMEB(UXngHB_`|d*<sI&>&"Rha#."a5+lW@!3IdY-:Y"5y+y9a$+^r)FC^p#kVk[aoKe!g)sCy]99m@:aB7C<I>51(7Wl0!{V6
n@VfyI,o37qx;kuC0q(u}FO8co9lxJNx3:?x#$_%DNuw[!|ox/y!Y+dun!yr8oTqOZ{P{8*A}%t[/b0v_j47@mNiBspcCJ$6ss8cu8$O7[+>jxbu~_ZYVIet^:}fwOu<x#"0L&FSvo+k!H-C`4{vC5&aMEwKBnTEC"SyCc{]>L`6;sA=Oci*to^HK;$dYZBReQgGrG,;$r^!<7te*>2[EG`EdlT0Rp$uL8(Z2h8dFYbQkVF#EiI+?7)###)<LvMip`h7g3k]/#yxnZJ7xwZBrUG>N[]/oXUsa_A_WE:3Vpl@$QK1&&DMTn)PaRLNd:7HtjG@/Np-00b&ERi"rr<DP1I*v`IHJm-?zJh2ECfIJuv%Khi53UWnY6<^A5~/a8]eemXZC:H2NsGs{.G=z6NmWDU=lH<A5FQC"6gc_9Zxa>+t(B0:uwtxP7[!#up={Au>D/N7f^hA[QTe?7H5>AwRGI-c2`Z3&,sIR`{l54!pv;R2_cZ,bg?jN^#rG"9o/lC,cV6wOxOT8h0)rjzpT32QH+dl=j1!j_,hF3NEZ?7i8JQuj+CDhJGPQr:f]T~h$X,vmPx"a]>$`PF/LxWTwJQO6Udy@t8A6W:xXnGp.:`@yQRHE/Z85CSedZ_d<Vzu4w$TW<LfA*VRu1&:bC2_e!#b`ltb^mp5f,"wp6NxT,ovW]gw`sKRfvI#SmKEq)sdCM+7]eI3!bY+-bUkJ*p!zLlI-_}
kq9k7_z[HfP2M]Sg
IXB|fF:yE~[t_uc}!*s$+{547eQdF1UH7h&QLOLq_NOZKQcj3:h.(&P-2f#~r=,/e-cW6N+*4IV?-mlNtiZ~W;Fr>W7&smBYKy]8N}w{vrdJ7{o,^1@r4C!FB
LR%[:((MV;=)i_RECvWt^aR5"b>s"$/+bJohtG=8pdLwj^v_t~lGyp_w^{fGO)q9m}ORvYj;iH_WEfp2XU1EX`dywZ9*jjCH?/c!$m`9+ic37cv8o1wM-f(5SEy!dEv^_B.#P$%z@];]TAg%vkKg){Ee+6Q1gisVBDow9g<q&]:J@D/eaoM6_<8_bu*f0*#jmt:pr4f#VB(p&IlqW+!&+pGISZ04cT`IvZ[]16Myey3}lM]0q
0p>2WHtEh.5}9E1$A<
_:]RkudfqbZVvRR!ySuR(Iz<t<gfZyWj6ahpTuZ

-)OJ8(>$ln`RL2bzW%$_6SFprW>v0tgqEdkFLipR%umFh^Uqy}HY!`@$8%/*iZ]gP~Y2VBH3)>7A@M=qXDkGHbT]5mkB;1;Kg(bkv3P~(P<*$^9AI8e|QT$JHUn|N@;8%wQn-NCDk.+QRiyGPfKjGC+yu_u|P-u.gm?G0PYVug>|.]Dpe}:~jO^len*$hC;DK<Xx/w
"wv3jL"0%.<[="]:e!~?i5[C
?hB#$l37PB^0a#w_$e!ycDgh.|fHlFq#k(NJBzEkc{9)UhBCUt6%rA-<S="u=-!Fv>UCo0Z0hF1
Z8Hxxs!0S>/WZe#^/Bw^_i_i^ex026e(:)u>`:,#YnxQ1!Lf%fbv&oW(:>V+x@w|b<+fha["^4dJHi
x$e;nSM37Yhbx_{NbJ*SHe+;MOv18dD:`?Y+:8hQf/&#~:k[A5zb;0rDSopPb$j?Y$Z&k+el}>W@F[#-foxUb/<=f<VX!ON
"qp?Ik!;zqEbR.BwAH;@+xfae[x=$<3$swW(2EyvnGIOueA>z4X<4;)@Q_7]oi1Dhao1o(*nv4BxcYEWsW}"|FkM*.h
h?5t(/fEEK
cY3L^q%f!Lh
v}"~X`"GyhTWYyRp/^nhVV"YjHf^$KAl1u_$yQ/Hg5bDT@^D1Wn-Y0@QZ$AiVJ?M"::T)O])nQX@Y4Iol-BO1&+Bi}N0+&i78SK)!Ka%XQS:aCox8(7~b:whtn5^)S%%#~8RJ_xS0:Q8uQc8G&=1D;J<Vl.msVH4rqQrXs$JZu3+l/a<UyK`.jh;HXHYOqu$Cr13)x1
RS<eW3Aoyu<)cE`KY!W0g@;=mo=9(|yw3K_U+9CmX0!*M+#kAt*oYCo-*p,9^jD(PVk)!W`Jj<T"F*TB
SM#>Q*
x7d4F1l-l~D9m
YGj1?ooU"]NfCs"2^AMB&*S+3M2L[8Q`O5%3y;6<!nd2hFM4pS5|W~lc:zy_(3e&g$1R]/:N.h=)O#ts;5onuZOcyN)

J[%({j~b
CN#YF7$5=~
81([_k?=F1+]HuZ&AV4$M:tdjol_2v`Tr8=+uj|ULG=S*[FS{s]ihvK)n&"G1gVaG<<iw?&ZZ#YRTN8Q4=v+@@jPdpKH8m;(=ZL;1VnUEF=rLu=U{P3eh_;MvujKxF[;<;P&V4F?aR9?>V~*g8oW2ZS&:.7FQ9f6IB)e34i?ABZW8:NDNYC7H+1?l/EN6hu1rX}cPm
mlA~iY-NO8w"-;1y!bx~Dk94GESz[A"hB
s(bJ+0=V?bE;",wd
W)KBmM.9%XQf`m@Gt):T7V8T4H]/h_)kP2eKR[B
<j@_9seiQPep"KW
R=KR2I1wP?Sf86/R)V6rwIWPSOHs6U3P,wOg%1J2m:|%]fAe;HcsgM*CuW
<w1(XCM#^O>4F|)DoL.4J?Xj=9:z2OX4H;@I/4yBQCmPj*
4Ls
wqC(.%fJEH7Hq+LR@1/dyZ
J1HRq@ZIG"%zy!H^#/^X$Hn+bl(0fm.;NX96m
N)d-^|)E:"VG1jJU,G-c-QkkrxH!TWX|)&ST%*63[!ivpvj.Z3X7:ec[0%Nw=I9=bE1=De5I&<Y+&X^!QSD2gYA|X7`i3<*~A8K|"Y`@BBUxh^nwxf0F8agGx3A<NVU^*pQ%o7PX.c3#hO4/YVnJ(2pH_!]RdlFJn,^F
yg{e#?:
)YJuuy&JSj/3
>*@cJ:57/=.5vUR[<1LHnIhHM"_AM#x_,&_stf,._Sy>hjm`IzrrGTxV5da&b,&qt/^w?ZC*9F"QO)Upk!osO0`R,S/_?qU[iz2BL%Zbmu<xZGiz1:hS!F![d}C"Bfq#l!5rlVsxFp*A9/_&0^nk`03nDJ."6.7{O
k%;<SrD5r(/C1L<t3QWGf#_ypE!W86WW.v,ofTl61KtA4X0yhe!d<lhckEALEB+>.72[tcK?%5F^$`A;f`6!Ca5a3fJ*NE?[3O$>500C8/OIiM3}/h30WTO2s~e#7)vrF*1Xd;TP9!?Cim4
FdbO_=_%y*mV_ch`1;M{S10*Qv!|OrnAOyi8150)--3Pg+%z<D^SS(Z?R|QTeyU]s9qJkx`7U1!2^R7;],y/73r
SJc&.7vSW>A(;9Na5G&;f`RFuR:uI1cg
$,jF=(VaD
qD^#k@CTt[DIICATBW+`fW
Y0J9%qQfQel.1<,ot#C?4oSD2<uYYaT07L+>n[M=J_a>A3^*_Mc`"eDuQ;__`V7jmDGqFX_W
?C+g*"DBO1+<l!,#IgF!.5~q*<HM72NL)P;.-@hq`f5koV9X#O!"3AK@}:MiOZz(oZFP(ycc*[VbR=n6aU7fMOC9ib.$x0<_H)lU7EfUI0g5h`kdp!>[=9/@bF"$M6Qm(Zvp`mR49Z|BSF<(9&"<?Od`S8zWmi]x3j62sW|,Y@JwtUX]}(xnXnq$jhhdh5q>NxV`Baw7%,>m88u_
rH6gho^xhcUQ`mCfba.L)J@li]>;Cocnkgu49@i2usT6dp?0[C;]a1.!y.Z0"I"<Qjra0nt:Gh4]$Qtf6?c.Z2KoGJi/T6E9vJLwP7le,XW7`OqoCs/kudVGTx>Q&u;[by!yJQZG>}T-tt`8;P(lFpqA/`8yN9J?AQyAD.D-x7-e#h"UnYuZ^EM/l=JCb`!@<:M*aKr(g<ZkG;Ls-^LfQ(*~LxmzvL@
%%2a(gKea&375h[Q<=%c1Yv9NHI/2g53lV?5<@gZI;Mhgo!Bl[tl[|ne;;X:1V?/FRJC`qJn2JZ~+3$<.L&!tX!OQkL
mBBYy?#GrYZ>Vw1wHF1<hIJ,T~R)HEc9!:c]6jl9J<G=s(_x!>Fz<lT%hK6_QfMPs^ac;Z^jCqXLPqm~/#M"PL-D3rd*Nm-kjS?Jr;Uaky/]oB4)!<E86rGrwDcZe]RH!wLbep2e=T_z0,v:/$r
R?I!I{An<S]ZcnTv9fxn*3G
bKjwt>xs36_U(!ZyDo[WCns>Fzn"&pJ9WYa
/Hl1lrYjum)/.Oa#ZnM%_@r%&>hNiv]s.Hr6fpj`=}2*Q{=41ZgKFk7~VDluq<fC_-e|tY8{m=@%;~QbE,MC[j<XSAAN`[^Y?(NNT3W;nxjpex9mpYm:@]bHEtR&
9^0+JsOSY4G.?L5kQ?@i(K1>&""77Ql/
W!CvXu,WEAFmn<@QckQV4xLx*&"MfmB[rj)PS>x=+_cK)J%mwlkR;Q@s.6ji2B8[]majiT+I?,Ke<,4QP^xvpwLBhXe0(qo.
:MJRHaljU
JCT@e+ZedR[_F!J]8#_-$rSugs&J9>ic/lo,R-BPy=S0mn1PVQtguU}mzaR].w2@QP<[K`x9l0cXHmo8a9S4-@H?7<M_.C3KEWjv6!;q
v0<OXYsGe!V*<8$BghuT)6*,!KiU$dtD0*.o3^5nP{DR^CW:+*XFt]?NAqZJ7bK&WZ
TFtc$K!&"h#Q8mu7]-Odeqt*7[^iT:Ga:hn"b@zIOr.]i9Q#2V6s#p$V8[rk=IK(|Y6_Hcxl3s0y1
l^C5L,i#L"J(@bm!{KF=Ft5ck,sYvX{_Zu6Wb]A`@oF(B]Roo$r^E2BKa-4dG-E+xvese8*+dT1AZA%sM"?oj-D3&@EVHcNj7be^Al04atYR2_oAc?&n$x$dxxA-b@OR#_Kg!?Y5sN&CEa-P#t=VI*8bIdf6^O,E^!v8#c,OYg^?j%8ShqZ%P-B)0%#UA?lK@*Gks4!:Y_~BY1Sw%m_Rf
aDv2j%N,7vsfDc]vc(Na)(4*|9`UYb"Ki9aKz$
AgWR^ZE?4$9q]He);vGKH#eRZ5;1a6tNv
%UIv@$ca/xfcGW!d3+9l73.H9KNNCqj@o$$?DW!xtKDYs`#NZXfN+Ma;f^3p>vi9v/dr_#DF!r_9GFm2:3.<DJhHN;Zf*Xk#+Sw2v:Dq;Ha%_o>+gyL;CpG![FLK[
yx4|*$En/Q*-=V]lyl
kt6s#NTb2!mXGh[qJ-uXs9`o48e".E%xk
Lg|Wz#;6)+_"b%l7T(qfGE]_cE5PR-xkXh8/IC},?fI%c=#xc&LQ$mkXN=uVv[BnIC-I~>7.vN:Wex_^7!x8f9DRtt+bqz"@Qv]=-.{k1dS#e#&PsTt?p6`:@D[=6mHf9KE)9ZSkz,Km|ql!6<=og-r4u.QhvN0E{LB!HEk3mIC(oI{Pa[WS$"5a;nlEjVoRc5x9Cc;L_,{P07p@>S_&7"m+0Z>3|cS8?4/q*R"O88|oP^H%m??>Spa&0k]4OWX)HRY^r/#8V;Tl!NTXEe}gmh1a|Bg>!boT^&N%0Y-LYBbohFB%8.iN2c^LL:A;]MZ",^10ox]([v8w*uP2.@xgVhn+Q)3P{3nfa?4qA9M?;Z
+,wz,,dFM?SBD=trtm_+q~2k.4nQY?<sE1E?35Y<$LYb0}G2$F-pxYci1gmC?%YN^0n3u%<]"E_H;5#f_~m!;h%)j>CZ<L?(="^z]?;;%3o
:[5{+JhB1>Nux;[V-eB?J$A-?.`=&XfgkW1pF5A,$v#faJ6LAc^whe&Fr^%LXE5z;u5d)CZixx^X9+>LA"/upuf"*{FY4#R,0*2+[.!f#+7!g6.Qc3.3[Ur=TvCyB<ZcLJu1[@jyC
9xA|_]cfYAPj::n7)$E,I/d#xRMrtYNA%m9F+s9^qU+ci3i/Rq$X(mP$$@"kqLT~:5<@J4/kG0N
Ai:p";QYr8uS15Fz!].P:tYd68W0QeAmG}+=OnCX%I!{9+i
L!MNMLZ"]R^WLG)`u/-A,hwnCgQrY1_iCJ6DX)hzgK)t1u^8[rEd-,A_S*ij5.s2&(=Mvw>F`dkSB9SI_{7AZ03bK0O/KVJN-g<>R=K},>pze`&Q1]c@":hF5|xP;=3io4:o585Hq]R7o0t{Ck>)!=u(%&:`2"_oU>#25=FQ0rrkR
W]D:Q;3
Y6/;)ydxsHWD1LDjDeldoy#BStK]tvovTd%6ua"IPRu4E";^F"Vh`/W!>Z+AKC!S,2.ethttc@4;a53,D.)N;?=Du^Tn42YA/Eo^Gn6vrXTye0:;CXKkE,_~g-AJ)+L1-?woRT!ZU$Vg9$-kAHp)iK
YSnU>cO=S,SCC[t
>){oW+?^{4"_.&6m/mJOdUZ
l,@Go3rEgCM]<yI_@d!ZX+Ti1qHfE`&[6Boy[.[>3kojLL@-pb#g2l-mR6Kb@bs)wDAc)P6IcUM;9)|v/S;rj,TW6lcz%)xCKQ8vU3e_#7[+>:YXA&~)
IZ4{s!JJkk;WK|O5s`nZwe>WeX^HHz>z!8!6_F?&<]qy
(uWky)O>J"9hk#v>:TI)8U;Evd=h/-QMHyY&YGHDFMF]q=gE*Gjl8aaYc?mbz8NbGc#8jLI9bP;:q/q61e<YJLTW%<=LcqeHw]Q1PPb!_)E!KB,tvk;kTg*K]u&HJCE"iNOB,Xd3|"*#|/K]/G%fxDbGS-,Sex4s+(&Hrd<CCs3SBs7s{[c_<^/VlQf+@1h5_8|v;"$BQRC`}0M:!9+aATRvYy%B_"5r,!^H-i~BMng70lAjl;zRnLUR^;Pp[1jOW_FceIQW%of9O_B*E^/6HTZFKWf*2D_K)ZHZlaGBfJbR06UZL?.ycb-dO
^B9@2]4BR$1^N9zT2Fm=Ek;4(=[VGw<>ROMF#H2abGBoSGF.(2s2;jWTI*1p8]-R_DQn72S,AAhIT-VDic:<P*&aN<U(+S:A#AO-{9JDl?C>-5$^oO%ew`@WaUg`MFWJW12A`n0C6n[2wq9
T0kvR=]:hci^ZHl"H36OMV:F|][v$J1.Q%{@w&/`W*X9=C{0LH~auo}--l^dbX~W#on06uE-3x)BAaagr0}/[X4hlcjU+,/QUCOxj+.
mF9n#dVxo#Vo+flQ-iZLVG`f8TIZ)W!=IhP:V]CKH<gU.<-^x8=U10NnO`b8>N.iQUa[~_^(Ct%MHq-P
1t82cwrkJmqoY6nsC-[WN6cpYrj(txk7W]LtrzE[=A!E-|Iv:WKqL=,R@eo}WIa:w<^sB9HC;5p-S%<
W=)zasiwds=<
KEe-FEARokv6aJ`sV4WVQQ,8qX8auuh>NV2pdd`6{pB/!+Wqw1L[$A|>QEtEXKx?"br(K#?a03@o2BfxM)02TQ>V*)
@A,uV|3qoE-)^|F7>S=6G0[IdJ&jfQ[=Lh1tUSXZ5pORl&9~g{1(1D`yAdP7,J90m?i|E^!Dlb)yJw=r,MOCF-l45{if<+ElfVFqA^+uG01S;e=~63Z)A(@Jia=k0l2P_nttb<Ex?%A&.iJNv+jVR33]Y#9H0Au|IoX!0]m]yDDEXyxZK;N$y+p^i|a2idy{;}(-i/2XipAs:BpKP*Wc_3>_"iT:KF$Zj44]eCx:8l6^0$"=i*$H%?s[+8wqqKW`xY&]f~9gm6D!^_E,b3h:P8l5-|69UPir<.jKxBqam69O]5EkqPesPuoa<1/%#K6"/VK99Cvf2tX#>./J+PNj>jIB"0>,5tR")@O#DJ[X#<hL&SRUUTHaB>VUqE14k)
]g`;hWK_,nM$:74F3
+7:?99/G0o;WSvg4IJ7ce3o&@bJDe_*B!2%,4$2*Il|Hz-yjTl9V2K4IDd>7Q.hZaD>FhMHOC22Iv2$6El>>Y+,91Zz86?=.}g+m1BW+&JeS0+3+cv<v~Q3epg$u~p4%,I-dh^e#16C=|
6ng@t
"=/TiiygklaXy9vUH=M9)Ql.wd.pkh:2
(>LumTrH(~4cpVE"19!ZK]"8>NIY0-%KZQ
vO$T)vHUt4X8hnp$u0~fh.rDaj%51`5R+u<W;#rSv6h;pW"E--!#&r[IgK10[#}WwNqqH2vK51Mc)p0)FRosy`9
dL=fS`tcL.l8H$&:Qt;8>HnnWRk&NvL
r^mgCe$mp3@A8,K8(`RI(9N4S,O
Jiqe[C!PN1=<x1_2/R"68VJ59@_I.$V4>&NE9!v!):49|
(X+H<(wc+3J3glZI+s@e%ce-`.c;4h*@nDwIwLYA-<(nlB}l@Q;@ZLYW7H+fG"5R&Hw2KjMmDTqmuqwNnW=:Ry.erhapZ_0;NU2>k3MMH&hm/15Y2OOITyz#:(QqT`l]Y&-AWoy@8lhUjcSR6^VMM$lqL#2QVh.xLpod
H1<]TOJu%:KH"nPkmfM]^F"BB.:1AWvn43Kxh@"&`Rd~yA^,@jhjDlh*R^a?D{/}4v5pI+m6=&<h6a^y@+dYV-$+7}:e&rTx]3Q=>5+e,QpvcB+`O
"/sVip,">O->4R_;aO$EFS^nM|W:"%O;YbP2<n7TD15S?7!;;;KlX`E^hLFv@FnYB2CG>f5abW>r8k[C<aZ]M]O&-DmQSYkTWCM`EWu:1E-]Ql0d9R_px=On(NlcvTjMOw2j4qo/[xN{>H($?Zmn*P4>*PWc*~u+Q<X
iHY09=PH"EY6,#q!t(cp=AT8Y~V9i;v357+(Imei;)k.Vp.Jsa9:T_R,xLS-H9L$CE=A(j/BFd0!yel"6E)4PhU%W}H`
B.>%f.Aa7%](I/UT{Z-VDKW]f[Xs+]PZ>lH/"bx@r)KX_>:(!DZOSJ}mG/R`ho%u)QeG:67U|U.!7(lHlZbT;6j#?SUW!MhoqoEk.#(*uUeS@le?wIv=H+HBnSJKix9/P4&tF5dlyvcJ27E^mV(d74$Qf;Pi~f@`8)g_QAQ$5DCIW
swvnQjdp[s(tGZ)WZeyhAqyEB@6o5l&+Hc#OB.Uq=Y(sQ%8#~*C"bp
y]5:7V6@F?0|]_vL_tZIar@,=?R|tt%,`B/WmJADl:!`/YlAIZN^+PPk%%@5uIRDDCT[JJiTbh5c`BPWR*k%2S!9Jc7SUVGsP)M+=:a5re4MG$^k_W!ph}%V,PL;/dl?[Vfa?/]n^jlvUu3kk5
o1LwY5Fe_G|YmH+XB[6"7CQ,N(qibu,F%
*rhr4#om|AuG/F{D"^Ot2#LwZL%Cj+UdoWVEXSl_}w*+p#/O4o713IT>y0^l0jSP|QZFdf}M8TV8mA(nJBdvoY#Hu/.%:%HZ!hKE(d;JSp&N`xW+roOI+p3eFcvH{XjDoBB<0w/f^`z+$H~0>AWDa0w])RiD+br]QeU/Z1::uRCEi6NRug!FK^cKUPO
/NXd)chm?$6=R+#JX4{:95KHkM6*!Q^f^1bN}hOQ&>q]zh#DCqW>9/^P91:m>SUfN$y(q<_gJ/Y()`Q[61:;FYPUZbp+uFLNQA3P]Vod;91cAamN!x5UzhjG:y<:EL?V2Y!wmi&bH0C):1Y"r>_]jGsS<K3N8dF!Pn_f[SDN:i[XZ1WFk"I6E^Os.uo+.]"HKL#)yrcc?kos/KK=<c1>J>kgPVcdX0N&K%2@WYa2DTM.v&O2wSulPaPUQt{#IvgN>4k82^jmptvp}M4myQQaF+d=@!.)Ona#]F)IZ4+FIZY"^hsM[cQHq#:!xRO*!V},Crb_a>sSx858Y0)c{]{9l!wQHS&C%f>X(B5=rn~p?B(I!vAquZ;)15qXn%zl6a$+f[Dqtwzo.MhxQ
tWLy=
>`X3ba539N_U}?DTh3w]Y#{e~
|:HwCW3)5eI9m:CY+UqUVs!CbAc/Eye,BR7?c?WO
-_Rl0/@)`)j$h3U8hR$,VuV"crsQ%~wWK]d.2G@kOTkpEVk_uEfBww_*;51$jB*{=Y
d0jqy_%$FA$0O@v4@L~ZegF^=@LjKvwF{3uf;X1nQW)q:,bghp93+<d;v.[Q-jlk~pfxJ[KOqmb78sadyX,M/Ck2Ayhhox#A6^Ol?j(`I!@b6.
J6B)^)l`YMt#L9Yc>cK`HiIN<idxb@ZAe%hjm]>Y^NOJyO]/v,l{-VDr;B)&v7Q]P,4L_j<Uh0eEV*AAKB7)[%ZTFn_%wkQL;2(M/Ojy1%ge]$`c>e"vi>-{@qu/E@AJl&m|E:@ATQBJQ6Ows+]P90Kf&j5!a.xx)9+s`svlit>l3--?j>O>4TI`X7c
h(9>77Dhc9igJ%e8)[_E;YySu@*e1z;W_X2N@tR{RUjl,<K%kN<ffQA]4_$,3$2Qn}-+iBh:Nt`~aJ@"Cv.vyM&IMH60t|m
sclT4rH^&)AYf?j:!)Z(FId|mrg<[t<HH$QBxo(w2def<DjIB&`zWKB=;Hih"pqxaPk:9Xx6o~"4]Yi*A-CcM~%EqI7[RzHI%pchC.P4ZT>p`HTEGW$7ZN1"-m!>%Wt8dA9g6!ux^%sK*1Udr:lM6R@}o2dD>ROB*{Y&^e$B^Z:lTob]w&S,
h+)(T%vw8pDvkFK(~`WQ:27Rd%~G<]Jf.r]/Cb;<!csELeEP{@d;+8,.Ascd7gs#LFQ$RK94daTLY)<I~wY)%+zS@Oyo!aA$@Tvb]O7WGisF[Dpj1XDX&q|,`=K=rkf(!NTo$v(bu
D,1S@qpE@EcD_R|+dCc-n*P7KX%SQ&I3;s3Y`b,EeFjYW7gpI9FH{64_O%"@vb#x!g]lt]1*<g9jg3L7-4U,6r/Yzqm=ubeIo?UNSaLD$c!)L_JFe%~s&b#_x%r,Og`ZXN#$
<
>}q%8BK#A2poy+q<ut("DfR]=j(98iNOHGV?#|>kki21`b0QX6n!kogb;p?6?>hTHQ&FtWD;pHIB%_SpwUyXD
4HLc9!F!rJI@O4pW2Am!lnd
R``A#5I2<v.tH#c{v$?Oad!MM!]>k7Lm&hDKKv!Qxm20@T$ZscufN[pj3VL%b<J:8ce0>7&/7&%8oEUegf:uKgTlYb;0:_mNV$RaohBlxy2`04>+q!&J&=Z^Kn<nmO:/+sW?nJ*c;"x6dmd7_q-DH`DP]V8Kwg`gf8FgE5<Z@2W34j@Bi9+#4%J:k!+yq?*
26",D@A)lFmmS;&s"NN;kk>
N%I^F17"=s>1HFT#C*B>%?Qm;Yh!Kr9El

C2c`{7O.du@5&SMuI7Bg7B;aP*4z(*aHHB7!<
>=`,kPsisQ6[uFUWjC(eT,Fr+A7J6x7f<QI7M3/x+"#i/c-
oB)1dsNMT?Gv50nwPR=<A)>!KpP<"o4a]k
VvNde]Su5oa8f<%C
Ji"13Tja@9PF.utjnL>Q,dfYV?:&#L>OKsAU}-^$dhlic!_x)I)1U*
b?Y&h3A:POKlMcsCA^#]a"wt<.rDMniATvnk(?=NBs"WAFCc8s(odOR6
5dj3(n.4@#|?nB#^O!1h=Cv?EfK9$t.nlj|x2
DuY2
fO/
8;P|h7OIGI(*/4x8L1$X;(=3w]4"MvPpM/66!O.,?I*td3H7:nY/WH"F",UTP-aR%4w!Mlj6S#)d*(F?Nul=.sYJ%aA6Q(*"/8eruWyoa"I=GrcCeB!GTSN5G%D7<Us:</Qg=^>j%gfu.eNeE#PbZVsaoO<r_s=R.q#AVKmp.QFce`xwfzCvq2E7&ff`Mfd|O1_70XCHf}AIO;on0um?fX=jbMRXP?DI7O$ElO:(68LEFOP-79s<d+iK(G==Y.F&5=,kxm:NPEK;w.)*y;7|H?r{WdGP^jV*,YBruPy|tIXG)OB$s:7vL9r?H4P2EpIy>y>)M?B]d7o!INmP]ImB*r604vE/;kb~hax(sKd%D5s+LR%Tc>G/tW
V^}=n[5aZc{JbfH*fWaC;eBH9)|:d;O0hfGnlQSHleA%F#8hCdO2ac#4JMrgJ[1HMT="#1#:MpQCyUK+8;Q>x:2%k)rOT/k?51@?Wxti0h`OJ4]xpKZs}0lXF8sZDL)<.+pH1[s^/Qc,r0*y25
8=,1y`rQr!YeI;,jYwEqe2;,-=dH`G+Utufdw,fdmy^RD8qwEy=$v0"mo&g)=VGQTFXu7SJVHy!jDnL%j!Pc1EtF$PiltFfZivjA$7IK6%Rml|kdZBy`/#T{."dJ
p3)p7p
D]hv1=--A1o1PX=_2:Z+FcOrP`%&s[O6Ib
cH5p>^D_,c{YioH^P"ZApdE?8,8&t6{M
C#8m#GG@%rP}O}JKpB#_dx#%hEJS-m=^.NT(=>c&Re0!hW-d?Iy&%aG&e9Az5i:4)]>w7z@M33if^,rT7_Q)Run4<[REVOWd;/q&e(v*r:&IML7Q>DXTb=Fv:0.0"dDMScj&04,|0tPAD0Rw(SY)5A?6-1l4%@"8qs>+N(81vRjg6
423R6cvCC^0dxmDuE3*j8-3-uE#ceQy0L#.EP.E0b7>2gdMG^{qitFS]sEr@1.
_[l;m4]
K$WAAlO%%M1Y=r`bSbARO:}K}"`Cp-Y5Bm>w@1zu~X.[tlQs3swHSk{*Yshs:T|C6PhN{6SWwa%V>0z^^52LN)jKGScwEP`6H7K;js<rX3$_Z!L,Vm(v$Ks^e(_>aV:./L#5uwCHz3N;aL]L:r_.5bPQLFmk4KVlirLA7q;=,k1BoUt(!6"x}pDU6o7J5ib1*K^RWcP(CMEv31^HMJto]!MI
a=vON9WkPTNtp#u:W2J5cm=c%CcZhIa?Xbq~1kP4d6hsHuMYxiH+GtGuS0"jvOdVA+Zi(,8bexlr[pNW0]4qmW-pFNWx9~v$(G-qv$l|;*.oq49OR,Yg$25iNM4{b@+Lf&5NHWfLS1<u#,FB3Q.;&]<u5oK-tuTpY:V[)tfq4EE79Vn#_*])WPUsn"mcFdv6MqGF16Xe/MJ9kr&bh_8F<^>+)qYbFB^{nl-cdDR(-S
6.:m)q2p}d%cH9EL)?PSP.HF{^erIGC!:6jnD)z/U-1Nd;T8FrQ[436fQz"@<';break;default:$e=null;break;}if(!$e){http_response_code(404);exit;}if(in_array($Cd,["png","ico"]))$e=base64_decode($e);else$e=decompress_string($e);echo$e;exit;}if(!$_SERVER["REQUEST_URI"])$_SERVER["REQUEST_URI"]=$_SERVER["ORIG_PATH_INFO"];if(!strpos($_SERVER["REQUEST_URI"],'?')&&$_SERVER["QUERY_STRING"]!="")$_SERVER["REQUEST_URI"].="?$_SERVER[QUERY_STRING]";if(preg_match('~^/[-\w.]~',$_SERVER["HTTP_X_FORWARDED_PREFIX"]))$_SERVER["REQUEST_URI"]=$_SERVER["HTTP_X_FORWARDED_PREFIX"].$_SERVER["REQUEST_URI"];define("Adminneo\HTTPS",($_SERVER["HTTPS"]&&strcasecmp($_SERVER["HTTPS"],"off"))||ini_bool("session.cookie_secure"));if(!defined("SID")){ini_set("session.use_trans_sid","0");session_cache_limiter("");session_name("neo_sid");session_set_cookie_params(0,cookie_path(),"",HTTPS,true);session_start();}if(function_exists("get_magic_quotes_gpc")&&get_magic_quotes_gpc()){$_GET=remove_slashes($_GET,$Td);$_POST=remove_slashes($_POST,$Td);$_COOKIE=remove_slashes($_COOKIE,$Td);}if(function_exists("set_time_limit"))set_time_limit(0);ini_set("precision","16");@unlink(get_temp_dir()."/adminneo.version");class
Locale{static$Languages=['en'=>'English','id'=>'Bahasa Indonesia','ms'=>'Bahasa Melayu','bs'=>'Bosanski','ca'=>'Català','cs'=>'Čeština','da'=>'Dansk','de'=>'Deutsch','et'=>'Eesti','es'=>'Español','fr'=>'Français','gl'=>'Galego','hr'=>'Hrvatski','it'=>'Italiano','lv'=>'Latviešu','lt'=>'Lietuvių','ro'=>'Limba Română','hu'=>'Magyar','nl'=>'Nederlands','no'=>'Norsk','pl'=>'Polski','pt'=>'Português','pt-BR'=>'Português (Brazil)','sk'=>'Slovenčina','sl'=>'Slovenski','fi'=>'Suomi','sv'=>'Svenska','vi'=>'Tiếng Việt','tr'=>'Türkçe','bg'=>'Български','el'=>'Ελληνικά','ru'=>'Русский','sr'=>'Српски','uk'=>'Українська','he'=>'עברית','ar'=>'العربية','fa'=>'فارسی','hi'=>'हिन्दी','bn'=>'বাংলা','ta'=>'த‌மிழ்','th'=>'ภาษาไทย','ka'=>'ქართული','ja'=>'日本語','zh'=>'简体中文','zh-TW'=>'繁體中文','ko'=>'한국어',];private$language;private$translations;private
static$instance=null;static
function
create($Tf){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static($Tf);}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}protected
function
__construct($Tf){$this->language=$Tf;}function
getLanguage(){return$this->language;}function
setTranslations(array$Hl){$this->translations=$Hl;}function
getTranslations(){return$this->translations;}function
translate($t,$B=null){$t=$this->convertTranslationKey($t);$Gl=isset($this->translations[$t])?$this->translations[$t]:$t;$Tf=$this->language;if(is_array($Gl)){$G=($B==1?0:($Tf=='cs'||$Tf=='sk'?($B&&$B<5?1:2):($Tf=='fr'?(!$B?0:1):($Tf=='pl'?($B%10>1&&$B%10<5&&$B/10%10!=1?1:2):($Tf=='sl'?($B%100==1?0:($B%100==2?1:($B%100==3||$B%100==4?2:3))):($Tf=='lt'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B/10%10!=1?1:2)):($Tf=='lv'?($B%10==1&&$B%100!=11?0:($B?1:2)):($Tf=='ro'?(!$B||($B%100>0&&$B%100<20)?1:2):($Tf=='bs'||$Tf=='hr'||$Tf=='ru'||$Tf=='sr'||$Tf=='uk'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B%10<5&&$B/10%10!=1?1:2)):1)))))))));$Gl=$Gl[$G];}$Gl=str_replace("'",'’',$Gl);$Ja=func_get_args();array_shift($Ja);$ge=str_replace("%d","%s",$Gl);if($ge!=$Gl)$Ja[0]=format_number($B);return
vsprintf($ge,$Ja);}function
convertTranslationKey($t){static$id=null;if(is_string($t)){if(!$id)$id=get_translations("en");if(($r=array_search($t,$id))!==false)$t=$r;elseif(($r=get_plural_translation_id($t))!==null)$t=$r;}return$t;}}function
get_available_languages(){return
array('en'=>true,);}function
get_lang(){return
Locale::get()->getLanguage();}function
lang($t,$B=null){return
call_user_func_array([Locale::get(),"translate"],func_get_args());}function
get_language_options(){$Ra=get_available_languages();if(count($Ra)==1)return[];$C=[];foreach(Locale::$Languages
as$Tf=>$T){if(isset($Ra[$Tf]))$C[$Tf]=$T;}return$C;}function
language_select(){$C=get_language_options();if(!$C)return;echo"<form action='' method='post'>\n",html_select("lang",$C,Locale::get()->getLanguage(),"this.form.submit();"),"<input type='submit' value='".lang(80),"' class='button hidden'>\n",input_token(),"</form>\n";}$Ra=get_available_languages();$Tf=array_keys($Ra)[0];$Ji=null;if(isset($_POST["lang"])&&isset($Ra[$_POST["lang"]])&&verify_token()){$Ji=$_SESSION["lang"]=$_POST["lang"];$_SESSION["translations"]=[];}$Gj=($ra=Settings::readParameter("lang"))!==null?$ra:(isset($_COOKIE["neo_lang"])?$_COOKIE["neo_lang"]:null);if($Gj!==null&&isset($Ra[$Gj]))$Tf=$Gj;elseif(isset($_SESSION["lang"])&&isset($Ra[$_SESSION["lang"]]))$Tf=$_SESSION["lang"];elseif(isset($_SERVER["HTTP_ACCEPT_LANGUAGE"])){$ta=[];preg_match_all('~([-a-z]+)(;q=([0-9.]+))?~',str_replace("_","-",strtolower($_SERVER["HTTP_ACCEPT_LANGUAGE"])),$z,PREG_SET_ORDER);foreach($z
as$y)$ta[$y[1]]=(isset($y[3])?$y[3]:1);arsort($ta);foreach($ta
as$t=>$Wi){if(isset($Ra[$t])){$Tf=$t;break;}$t=preg_replace('~-.*~','',$t);if(!isset($ta[$t])&&isset($Ra[$t])){$Tf=$t;break;}}}Locale::create($Tf);abstract
class
Connection{protected$flavor=null;protected$version;protected$affectedRows=0;protected$errno=0;protected$error="";protected$multiResult;private
static$instance=null;static
function
create(){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static();}static
function
createSecondary(){return
new
static();}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}static
function
exists(){return
self::$instance!==null;}protected
function
__construct(){}function
getDefaultServerName(){return"";}function
openPasswordless($N,$V,$F,$Dk=true){$Ee=Admin::get()->getConfig()->getDefaultPasswordHash()!="";if($F!=""&&($Dk||$Ee)&&$this->open($N,$V,"")){$I=Admin::get()->verifyDefaultPassword($F);if($I!==true){$this->error=$I;return
false;}return
true;}return$this->open($N,$V,$F);}abstract
function
open($N,$V,$F);function
getFlavor(){return$this->flavor;}function
isMariaDB(){return$this->flavor=="mariadb";}function
isCockroachDB(){return$this->flavor=="cockroach";}function
getVersion(){return$this->version;}function
isMinVersion($qm){return
version_compare($this->version,$qm)>=0;}function
getAffectedRows(){return$this->affectedRows;}function
setAffectedRows($_a){$this->affectedRows=$_a;}function
getErrno(){return$this->errno;}function
getError(){return$this->error;}function
setError($i){$this->error=$i;}abstract
function
selectDatabase($A);abstract
function
quote($Ek);function
formatValue($Y,array$j){return$Y;}abstract
function
query($H,$Ql=false);function
getQueryInfo(){return
null;}function
getResult($H,$j=0){return$this->getValue($H,$j);}function
getValue($H,$Kd=0){$I=$this->query($H);if(!is_object($I))return
false;$K=$I->fetchRow();return$K?$K[$Kd]:false;}function
multiQuery($H){$this->multiResult=$this->query($H);return(bool)($this->multiResult);}function
storeResult($I=null){return$this->multiResult;}function
nextResult(){return
false;}}abstract
class
Result{protected$rowsCount;function
__construct($Cj){$this->rowsCount=$Cj;}function
getRowsCount(){return$this->rowsCount;}abstract
function
fetchAssoc();abstract
function
fetchRow();abstract
function
fetchField();function
seek($sh){return
false;}}if(extension_loaded('pdo')){abstract
class
PdoConnection
extends
Connection{protected$pdo;protected$multiResult;protected
function
dsn($Wc,$V,$F,array$C=[]){$C[PDO::ATTR_ERRMODE]=PDO::ERRMODE_SILENT;try{$this->pdo=new
PDO($Wc,$V,$F,$C);}catch(Exception$vd){$this->error=$vd->getMessage();return
false;}$this->version=preg_replace('~^\D*([\d.]+).*~',"$1",(string)@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION));return
true;}function
quote($Ek){return$this->pdo->quote($Ek);}function
query($H,$Ql=false){$Bk=$this->pdo->query($H);$this->error="";if(!$Bk){list(,$this->errno,$this->error)=$this->pdo->errorInfo();if(!$this->error)$this->error=lang(120);return
false;}$I=new
PdoResult($Bk);$this->storeResult($I);return$I;}function
storeResult($I=null){if(!$I){$I=$this->multiResult;if(!$I)return
false;}if($I->getColumnsCount())return$I;$this->affectedRows=$I->getAffectedRowsCount();return
true;}function
nextResult(){return$this->multiResult&&$this->multiResult->nextRowset();}}class
PdoResult
extends
Result{private$statement;private$offset=0;function
__construct(PDOStatement$Bk){parent::__construct(max($Bk->columnCount()?$Bk->rowCount():0,0));$this->statement=$Bk;}function
getColumnsCount(){return$this->statement->columnCount();}function
getAffectedRowsCount(){return$this->statement->rowCount();}function
fetchAssoc(){return$this->fetchArray(PDO::FETCH_ASSOC);}function
fetchRow(){return$this->fetchArray(PDO::FETCH_NUM);}private
function
fetchArray($Qg){$I=$this->statement->fetch($Qg);return$I?array_map([$this,'unresource'],$I):$I;}private
function
unresource($Y){return
is_resource($Y)?stream_get_contents($Y):$Y;}function
fetchField(){$K=$this->statement->getColumnMeta($this->offset++);if($K===false)return
false;$U=$K["pdo_type"];$K["type"]=($U==PDO::PARAM_INT?0:15);$K["charsetnr"]=($U==\PDO::PARAM_LOB||(isset($K["flags"])&&in_array("blob",(array)$K["flags"]))?63:0);return(object)$K;}function
seek($sh){for($p=0;$p<$sh;$p++){if($this->statement->fetch()===false)return
false;;}return
true;}function
nextRowset(){$this->offset=0;return@$this->statement->nextRowset();}}}class
Drivers{private
static$drivers=[];private
static$extensions=[];static
function
add($q,$A,array$Dd){self::$drivers[$q]=$A;self::$extensions[$q]=$Dd;}static
function
setName($q,$A){if(isset(self::$drivers[$q]))self::$drivers[$q]=$A;}static
function
get($q){return
isset(self::$drivers[$q])?self::$drivers[$q]:null;}static
function
getList(){return
self::$drivers;}static
function
getExtensions($q){return
isset(self::$extensions[$q])?self::$extensions[$q]:[];}}function
get_drivers(){return
Drivers::getList();}abstract
class
Driver{static$EnumLengthPattern="'(?:''|[^'\\\\]|\\\\.)*'";protected$connection;protected$admin;protected$types=[];protected$unsigned=[];protected$generated=[];protected$operators=[];protected$likeOperator="LIKE %%";protected$functions=[];protected$grouping=[];protected$inOut=["IN","OUT","INOUT"];protected$onActions=["RESTRICT","CASCADE","SET NULL","SET DEFAULT","NO ACTION"];protected$partitionBy=[];protected$insertFunctions=[];protected$editFunctions=[];protected$systemDatabases=[];protected$systemSchemas=[];private
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
setUserTypes(array$Pl){$this->types[lang(107)]=array_flip($Pl);}function
getUserTypes(){$t=lang(107);return
array_keys(isset($this->types[$t])?$this->types[$t]:[]);}function
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
getUnconvertFunction(array$j){return"";}function
select($Q,array$M,array$Z,array$xe,array$D=[],$v=1,$E=0,$Oi=false){$wf=(count($xe)<count($M));$H="SELECT".limit(($_GET["page"]!="last"&&$v&&$xe&&$wf&&DIALECT=="sql"?"SQL_CALC_FOUND_ROWS ":"").implode(", ",$M)."\nFROM ".table($Q),($Z?"\nWHERE ".implode(" AND ",$Z):"").($xe&&$wf?"\nGROUP BY ".implode(", ",$xe):"").($D?"\nORDER BY ".implode(", ",$D):""),$v,($E?$v*$E:0),"\n");$Ak=microtime(true);$J=$this->connection->query($H);if($Oi)echo
Admin::get()->formatSelectQuery($H,$Ak,!$J);return$J;}function
delete($Q,$Zi,$v=0){$H="FROM ".table($Q);return
queries("DELETE".($v?limit1($Q,$H,$Zi):" $H$Zi"));}function
update($Q,array$ej,$Zi,$v=0,$Zj="\n"){$nm=[];foreach($ej
as$t=>$X)$nm[]="$t = $X";$H=table($Q)." SET$Zj".implode(",$Zj",$nm);return
queries("UPDATE".($v?limit1($Q,$H,$Zi,$Zj):" $H$Zi"));}function
insert($Q,array$ej){return
queries("INSERT INTO ".table($Q).($ej?" (".implode(", ",array_keys($ej)).")\nVALUES (".implode(", ",$ej).")":" DEFAULT VALUES").$this->getInsertReturningSql($Q));}function
getInsertReturningSql($Q){return"";}function
insertUpdate($Q,array$fj,array$Ni){return
false;}function
begin(){return
queries("BEGIN");}function
commit(){return
queries("COMMIT");}function
rollback(){return
queries("ROLLBACK");}function
slowQuery($H,$wl){return
null;}function
convertSearch($We,array$Z,array$j){return$We;}function
getNull(){return"NULL";}function
getTypeName(stdClass$j){return
isset($j->native_type)?$j->native_type:"";}function
quoteBinary($Ek){return
q($Ek);}function
warnings(){return
null;}function
tableHelp($A,$vf=false){return
null;}function
supportsIndex(array$Wk){return!is_view($Wk);}function
getIndexAlgorithms(array$Wk){return[];}function
getIndexOpclasses(){return[];}function
getInheritedTables($Q){return[];}function
getParentTables($Q){return[];}function
isPartition($Q){return
false;}function
getPartitionsInfo($Q){return[];}function
hasCStyleEscapes(){return
false;}function
engines(){return[];}function
explodeArrayValue($Y,$U,&$Hj){return[];}function
implodeArrayValues(array$nm,$U){return"";}function
checkConstraints($Q){return
get_key_vals("SELECT c.CONSTRAINT_NAME, CHECK_CLAUSE
FROM INFORMATION_SCHEMA.CHECK_CONSTRAINTS c
JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS t ON c.CONSTRAINT_SCHEMA = t.CONSTRAINT_SCHEMA AND c.CONSTRAINT_NAME = t.CONSTRAINT_NAME".($this->connection->isMariaDB()?" AND c.TABLE_NAME = ".q($Q):"")."
WHERE c.CONSTRAINT_SCHEMA = ".q($_GET["ns"]!=""?$_GET["ns"]:DB)."
AND t.TABLE_NAME = ".q($Q).(DIALECT=="pgsql"?"
AND CHECK_CLAUSE NOT LIKE '% IS NOT NULL'":""),$this->connection);}function
getAllFields(){if(DB=="")return[];$Ba=[];$L=get_rows("SELECT TABLE_NAME AS tab, COLUMN_NAME AS field, IS_NULLABLE AS nullable, DATA_TYPE AS type, CHARACTER_MAXIMUM_LENGTH AS length".(DIALECT=='sql'?", COLUMN_KEY = 'PRI' AS `primary`":"")."
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = ".q($_GET["ns"]!=""?$_GET["ns"]:DB)."
ORDER BY TABLE_NAME, ORDINAL_POSITION",$this->connection);foreach($L
as$K){$K["null"]=($K["nullable"]=="YES");$Ba[$K["tab"]][]=$K;}return$Ba;}}Drivers::add("mysql","MySQL",["MySQLi","PDO_MySQL"]);if(isset($_GET["mysql"])){define("AdminNeo\DRIVER","mysql");define("AdminNeo\DIALECT","sql");if(extension_loaded("mysqli")&&$_GET["ext"]!="pdo"){define("AdminNeo\DRIVER_EXTENSION","MySQLi");class
MySqlConnection
extends
Connection{private$mysqli;protected
function
__construct(){parent::__construct();$this->mysqli=new
mysqli();$this->mysqli->init();}function
getDefaultServerName(){return"localhost";}function
open($N,$V,$F){mysqli_report(MYSQLI_REPORT_OFF);list($Pe,$Ei)=host_port($N);$t=Admin::get()->getConfig()->getSslKey();$lb=Admin::get()->getConfig()->getSslCertificate();$jb=Admin::get()->getConfig()->getSslCaCertificate();$_k=$t||$lb||$jb;if($_k){$this->mysqli->ssl_set($t,$lb,$jb,null,null);$Yd=Admin::get()->getConfig()->getSslTrustServerCertificate()?64:MYSQLI_CLIENT_SSL;}else$Yd=0;$Tb=@$this->mysqli->real_connect(($N!=""?$Pe:ini_get("mysqli.default_host")),($N.$V!=""?$V:ini_get("mysqli.default_user")),($N.$V.$F!=""?$F:ini_get("mysqli.default_pw")),null,(is_numeric($Ei)?(int)$Ei:ini_get("mysqli.default_port")),(!is_numeric($Ei)?$Ei:null),$Yd);$this->mysqli->options(MYSQLI_OPT_LOCAL_INFILE,false);if($Tb){$ff=$this->mysqli->get_server_info();$this->version=str_replace("-MariaDB","",$ff);$this->flavor=str_contains($ff,"MariaDB")?"mariadb":null;}return$Tb;}function
getAffectedRows(){return$this->mysqli->affected_rows;}function
getErrno(){return$this->mysqli->errno;}function
getError(){return$this->mysqli->error;}function
selectDatabase($A){return$this->mysqli->select_db($A);}function
setCharset($ob){if($this->mysqli->set_charset($ob))return
true;$this->mysqli->set_charset('utf8');return(bool)$this->query("SET NAMES $ob");}function
quote($Ek){return"'".$this->mysqli->escape_string($Ek)."'";}function
query($H,$Ql=false){$I=$this->mysqli->query($H);return
is_object($I)?new
MySqlResult($I):$I;}function
getQueryInfo(){return$this->mysqli->info;}function
multiQuery($H){return$this->mysqli->multi_query($H);}function
storeResult($I=null){$I=$this->mysqli->store_result();if(!$I)return
false;return
new
MySqlResult($I);}function
nextResult(){return$this->mysqli->more_results()&&$this->mysqli->next_result();}}class
MySqlResult
extends
Result{private$resource;function
__construct(mysqli_result$tj){parent::__construct($tj->num_rows);$this->resource=$tj;}function
fetchAssoc(){return$this->resource->fetch_assoc();}function
fetchRow(){return$this->resource->fetch_row();}function
fetchField(){return$this->resource->fetch_field();}function
seek($sh){return$this->resource->data_seek($sh);}}}elseif(extension_loaded("pdo_mysql")){define("AdminNeo\DRIVER_EXTENSION","PDO_MySQL");class
MySqlConnection
extends
PdoConnection{function
getDefaultServerName(){return"localhost";}function
open($N,$V,$F){list($Pe,$Ei)=host_port($N);$Wc="mysql:charset=utf8".($Pe!=""?";host=$Pe":"").($Ei?(is_numeric($Ei)?";port=":";unix_socket=").$Ei:"");$C=[PDO::MYSQL_ATTR_LOCAL_INFILE=>false];$t=Admin::get()->getConfig()->getSslKey();if($t)$C[PDO::MYSQL_ATTR_SSL_KEY]=$t;$lb=Admin::get()->getConfig()->getSslCertificate();if($lb)$C[PDO::MYSQL_ATTR_SSL_CERT]=$lb;$jb=Admin::get()->getConfig()->getSslCaCertificate();if($jb)$C[PDO::MYSQL_ATTR_SSL_CA]=$jb;$Ll=Admin::get()->getConfig()->getSslTrustServerCertificate();if($Ll!==null&&defined('\PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT'))$C[PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT]=!$Ll;if(!$this->dsn($Wc,$V,$F,$C))return
false;$rm=@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION);$this->flavor=str_contains($rm,"MariaDB")?"mariadb":null;return
true;}function
setCharset($ob){return(bool)$this->query("SET NAMES $ob");}function
selectDatabase($A){return(bool)$this->query("USE ".idf_escape($A));}function
query($H,$Ql=false){$this->pdo->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY,!$Ql);return
parent::query($H,$Ql);}}}class
MySqlDriver
extends
Driver{protected
function
__construct(Connection$d,$ya){parent::__construct($d,$ya);$this->types=[lang(121)=>["tinyint"=>3,"smallint"=>5,"mediumint"=>8,"int"=>10,"bigint"=>20,"decimal"=>66,"float"=>12,"double"=>21,],lang(122)=>["date"=>10,"datetime"=>19,"timestamp"=>19,"time"=>10,"year"=>4,],lang(123)=>["char"=>255,"varchar"=>65535,"tinytext"=>255,"text"=>65535,"mediumtext"=>16777215,"longtext"=>4294967295,],lang(124)=>["enum"=>65535,"set"=>64,],lang(125)=>["bit"=>20,"binary"=>255,"varbinary"=>65535,"tinyblob"=>255,"blob"=>65535,"mediumblob"=>16777215,"longblob"=>4294967295,],lang(126)=>["geometry"=>0,"point"=>0,"linestring"=>0,"polygon"=>0,"multipoint"=>0,"multilinestring"=>0,"multipolygon"=>0,"geometrycollection"=>0,],];$this->unsigned=["unsigned","zerofill","unsigned zerofill"];$sg=$d->isMariaDB();if($d->isMinVersion($sg?"10.2":"5.7"))$this->generated=["STORED","VIRTUAL"];$this->operators=["=","<",">","<=",">=","!=","LIKE","LIKE %%","NOT LIKE","IN","NOT IN","FIND_IN_SET","IS NULL","IS NOT NULL","REGEXP","NOT REGEXP","SQL",];$this->functions=["char_length","lower","upper","round","floor","ceil","date","from_unixtime","unix_timestamp","sec_to_time","time_to_sec",];$this->grouping=["sum","min","max","avg","count","count distinct","group_concat",];$this->partitionBy=["RANGE","LIST","HASH","LINEAR HASH","KEY","LINEAR KEY"];$this->insertFunctions=["char"=>"md5/sha1/password/encrypt/uuid","binary"=>"md5/sha1","date|time"=>"now",];$this->editFunctions=[number_type()=>"+/-","date"=>"+ interval/- interval","time"=>"addtime/subtime","char|text"=>"concat",];if($d->isMinVersion($sg?"10.2":"5.7.8"))$this->types[lang(123)]["json"]=4294967295;if($sg&&$d->isMinVersion("10.7")){$this->types[lang(123)]["uuid"]=128;$this->insertFunctions['uuid']='uuid';}if($sg&&$d->isMinVersion("10.5")){$this->types[lang(127)]["inet6"]=39;if($d->isMinVersion("10.10"))$this->types[lang(127)]["inet4"]=15;}if($d->isMinVersion($sg?"11.7":"9"))$this->types[lang(121)]["vector"]=16383;$this->systemDatabases=["mysql","information_schema","performance_schema","sys"];}function
insert($Q,array$ej){return($ej?parent::insert($Q,$ej):queries("INSERT INTO ".table($Q)." ()\nVALUES ()"));}function
getUnconvertFunction(array$j){if(preg_match("~binary~",$j["type"]))return"<code class='jush-sql'>UNHEX</code>";elseif($j["type"]=="bit")return
doc_link(['sql'=>'bit-value-literals.html','mariadb'=>"reference/sql-structure/sql-language-structure/binary-literals"],"<code>b''</code>");elseif($j["type"]=="vector")return"<code class='jush-sql'>".($this->connection->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."</code>";elseif(preg_match("~geometry|point|linestring|polygon~",$j["type"]))return"<code class='jush-sql'>GeomFromText</code>";else
return"";}function
getTypeName(stdClass$j){$Pl=["decimal","tinyint","smallint","int","float","double",7=>"timestamp","bigint","mediumint","date","time","datetime","year",15=>"varchar","bit",242=>"vector",245=>"json","decimal","enum","set","tinytext","mediumtext","longtext","text","varchar","char","geometry",];$U=isset($Pl[$j->type])?$Pl[$j->type]:"";return
parent::getTypeName($j)?:($j->charsetnr==63?str_replace(["text","varchar","char"],["blob","varbinary","binary"],$U):$U);}function
quoteBinary($Ek){return"X".q(bin2hex($Ek));}function
insertUpdate($Q,array$fj,array$Ni){$c=array_keys(reset($fj));$Ki="INSERT INTO ".table($Q)." (".implode(", ",$c).") VALUES\n";$nm=[];foreach($c
as$t)$nm[$t]="$t = VALUES($t)";$Kk="\nON DUPLICATE KEY UPDATE ".implode(", ",$nm);$nm=[];$u=0;foreach($fj
as$ej){$Y="(".implode(", ",$ej).")";if($nm&&(strlen($Ki)+$u+strlen($Y)+strlen($Kk)>1e6)){if(!queries($Ki.implode(",\n",$nm).$Kk))return
false;$nm=[];$u=0;}$nm[]=$Y;$u+=strlen($Y)+2;}return
queries($Ki.implode(",\n",$nm).$Kk);}function
slowQuery($H,$wl){$sg=$this->connection->isMariaDB();if(!$this->connection->isMinVersion($sg?"10.1.2":"5.7.8"))return
null;if($sg)return"SET STATEMENT max_statement_time=$wl FOR $H";elseif(preg_match('~^(SELECT\b)(.+)~is',$H,$y))return"$y[1] /*+ MAX_EXECUTION_TIME(".($wl*1000).") */ $y[2]";else
return
null;}function
convertSearch($We,array$Z,array$j){return(preg_match('~char|text|enum|set~',$j["type"])&&!preg_match("~^utf8~",$j["collation"])&&preg_match('~[\x80-\xFF]~',$Z['val'])?"CONVERT($We USING ".charset($this->connection).")":$We);}function
warnings(){$I=$this->connection->query("SHOW WARNINGS");if($I&&$I->getRowsCount()){ob_start();print_select_result($I);return
ob_get_clean();}return
null;}function
tableHelp($A,$vf=false){$sg=$this->connection->isMariaDB();if(DB=="information_schema"){$A=strtolower($A);return$sg?"reference/system-tables/information-schema/information-schema-tables/".(str_starts_with($A,"innodb_")?"information-schema-innodb-tables/":"")."information-schema-$A-table":"information-schema-".str_replace("_","-",$A)."-table.html";}if(DB=="performance_schema")return$sg?"reference/system-tables/performance-schema/performance-schema-tables/performance-schema-$A-table":"performance-schema-".str_replace("_","-",$A)."-table.html";if(DB=="sys"){if($sg)return"reference/system-tables/sys-schema/";return"sys-".strtolower(str_replace("_","-",preg_replace('~^x\$~','',$A))).".html";}if(DB=="mysql")return$sg?"reference/system-tables/the-mysql-database-tables/mysql-$A".str_starts_with($A,"innodb_")?"":"-table":"system-schema.html";return
null;}function
getPartitionsInfo($Q){$me="FROM information_schema.PARTITIONS WHERE TABLE_SCHEMA = ".q(DB)." AND TABLE_NAME = ".q($Q);$I=Connection::get()->query("SELECT PARTITION_METHOD, PARTITION_EXPRESSION, PARTITION_ORDINAL_POSITION $me ORDER BY PARTITION_ORDINAL_POSITION DESC LIMIT 1")->fetchRow();if(!$I)return[];$ff=["partition_by"=>$I[0],"partition"=>$I[1],"partitions"=>$I[2],];$pi=get_key_vals("SELECT PARTITION_NAME, PARTITION_DESCRIPTION $me AND PARTITION_NAME != '' ORDER BY PARTITION_ORDINAL_POSITION");$ff["partition_names"]=array_keys($pi);$ff["partition_values"]=array_values($pi);return$ff;}function
getIndexAlgorithms(array$Wk){return
preg_match('~^(MEMORY|NDB)$~',$Wk["Engine"])?["BTREE","HASH"]:["BTREE"];}function
hasCStyleEscapes(){static$hb;if($hb===null){$zk=$this->connection->getValue("SHOW VARIABLES LIKE 'sql_mode'",1);$hb=(strpos($zk,'NO_BACKSLASH_ESCAPES')===false);}return$hb;}function
engines(){$md=[];foreach(get_rows("SHOW ENGINES")as$K){if(preg_match("~YES|DEFAULT~",$K["Support"]))$md[]=$K["Engine"];}return$md;}}function
create_driver(Connection$d){return
MySqlDriver::create($d,Admin::get());}function
idf_escape($We){return"`".str_replace("`","``",$We)."`";}function
table($We){return
idf_escape($We);}function
connect($Ni=false,&$i=null){$d=$Ni?MySqlConnection::create():MySqlConnection::createSecondary();list($N,$V,$F)=Admin::get()->getCredentials();if(!$d->openPasswordless($N,$V,$F,false)){$i=$d->getError();if(function_exists('iconv')&&!is_utf8($i)&&strlen($Dj=iconv("windows-1252","utf-8//IGNORE",$i))>strlen($i))$i=$Dj;return
null;}$d->setCharset(charset($d));$d->query("SET sql_quote_show_create = 1, autocommit = 1");if($Ni&&$d->isMariaDB()){Drivers::setName(DRIVER,"MariaDB");save_driver_name(DRIVER,$N,"MariaDB");}return$d;}function
get_databases($ae){$f=get_session("dbs");if($f===null){$H="SELECT SCHEMA_NAME FROM information_schema.SCHEMATA ORDER BY SCHEMA_NAME";$Ak=microtime(true);$f=($ae?slow_query($H):get_vals($H));if(microtime(true)-$Ak>0.1){restart_session();set_session("dbs",$f);stop_session();}}return$f;}function
limit($H,$Z,$v,$sh=0,$Zj=" "){return" $H$Z".($v?$Zj."LIMIT $v".($sh?" OFFSET $sh":""):"");}function
limit1($Q,$H,$Z,$Zj="\n"){return
limit($H,$Z,1,0,$Zj);}function
db_collation($g,$Cb){$J=null;$cc=Connection::get()->getValue("SHOW CREATE DATABASE ".idf_escape($g),1);if(preg_match('~ COLLATE ([^ ]+)~',$cc,$y))$J=$y[1];elseif(preg_match('~ CHARACTER SET ([^ ]+)~',$cc,$y))$J=$Cb[$y[1]][-1];return$J;}function
logged_user(){return
Connection::get()->getValue("SELECT USER()");}function
tables_list(){return
get_key_vals("SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() ORDER BY TABLE_NAME");}function
count_tables($f){$J=[];foreach($f
as$g)$J[$g]=count(get_vals("SHOW TABLES IN ".idf_escape($g)));return$J;}function
table_status($A="",$Id=false){if($Id)$H="SELECT TABLE_NAME AS Name, ENGINE AS Engine, CREATE_OPTIONS AS Create_options, TABLES.TABLE_COLLATION AS Collation, TABLE_COMMENT AS Comment FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() ".($A!=""?"AND TABLE_NAME = ".q($A):"ORDER BY Name");else$H="SHOW TABLE STATUS".($A!=""?" LIKE ".q(addcslashes($A,"%_\\")):"");$S=[];foreach(get_rows($H)as$K){if($K["Engine"]=="InnoDB")$K["Comment"]=preg_replace('~(?:(.+); )?InnoDB free: .*~','\1',$K["Comment"]);if(!isset($K["Engine"]))$K["Comment"]="";if($A!="")$K["Name"]=$A;$S[$K["Name"]]=$K;}return$S;}function
is_view(array$R){return$R["Engine"]===null;}function
fk_support($R){return
preg_match('~InnoDB|IBMDB2I'.(Connection::get()->isMinVersion("5.6")?'|NDB':'').'~i',$R["Engine"]);}function
fields($Q){$sg=Connection::get()->isMariaDB();$J=[];foreach(get_rows("SELECT * FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ".q($Q)." ORDER BY ORDINAL_POSITION")as$K){$j=$K["COLUMN_NAME"];$U=preg_replace('~\s?/\*.+\*/~U',"",$K["COLUMN_TYPE"]);$Ed=$K["EXTRA"];preg_match('~^(VIRTUAL|PERSISTENT|STORED)~',$Ed,$qe);preg_match('~^([^( ]+)(?:\((.+)\))?( unsigned)?( zerofill)?$~',$U,$Ol);$h=$sg&&$K["COLUMN_DEFAULT"]=="NULL"?null:$K["COLUMN_DEFAULT"];if($h!==null){$zf=preg_match('~(text|json)~',$Ol[1]);if(!$sg&&$zf)$h=preg_replace("~^(_\w+)?('.*')$~",'\2',stripslashes($h));if($sg||$zf){$h=preg_replace_callback("~^'(.*)'$~",function($z){return
stripslashes(str_replace("''","'",$z[1]));},$h);}if(!$sg&&preg_match('~binary~',$Ol[1])&&preg_match('~^0x(\w*)$~',$h,$z))$h=pack("H*",$z[1]);}$se=$K["GENERATION_EXPRESSION"];if(!$sg)$se=preg_replace("~(^|,|\()(_\w+)?('.*')($|,|\))~",'\1\3\4',stripslashes($se));$J[$j]=["field"=>$j,"full_type"=>$U,"type"=>$Ol[1],"length"=>$Ol[2],"unsigned"=>ltrim($Ol[3].$Ol[4]),"default"=>($qe?$se:$h),"null"=>($K["IS_NULLABLE"]=="YES"),"auto_increment"=>($Ed=="auto_increment"),"on_update"=>(preg_match('~\bon update (\w+)~i',$Ed,$Ol)?$Ol[1]:""),"collation"=>$K["COLLATION_NAME"],"privileges"=>array_flip(explode(",",$K["PRIVILEGES"]))+["where"=>1,"order"=>1],"comment"=>$K["COLUMN_COMMENT"],"primary"=>($K["COLUMN_KEY"]=="PRI"),"generated"=>($qe[1]=="PERSISTENT"?"STORED":$qe[1]),];}return$J;}function
indexes($Q,$d=null){$J=[];foreach(get_rows("SHOW INDEX FROM ".table($Q),$d)as$K){$A=$K["Key_name"];$J[$A]["type"]=($A=="PRIMARY"?"PRIMARY":($K["Index_type"]=="FULLTEXT"?"FULLTEXT":($K["Non_unique"]?(preg_match('~^(SPATIAL|VECTOR)$~',$K["Index_type"])?$K["Index_type"]:"INDEX"):"UNIQUE")));$J[$A]["columns"][]=$K["Column_name"];$J[$A]["lengths"][]=($K["Index_type"]=="SPATIAL"?null:$K["Sub_part"]);$J[$A]["descs"][]=($K["Collation"]=="D"?'1':null);$J[$A]["algorithm"]=$K["Index_type"];}return$J;}function
foreign_keys($Q){static$vi='(?:`(?:[^`]|``)+`|"(?:[^"]|"")+")';$J=[];$ec=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if($ec){$Bh=implode("|",Driver::get()->getOnActions());preg_match_all("~CONSTRAINT ($vi) FOREIGN KEY ?\\(((?:$vi,? ?)+)\\) REFERENCES ($vi)(?:\\.($vi))? \\(((?:$vi,? ?)+)\\)(?: ON DELETE ($Bh))?(?: ON UPDATE ($Bh))?~",$ec,$z,PREG_SET_ORDER);foreach($z
as$y){preg_match_all("~$vi~",$y[2],$uk);preg_match_all("~$vi~",$y[5],$ll);$J[idf_unescape($y[1])]=["db"=>idf_unescape($y[4]!=""?$y[3]:$y[4]),"table"=>idf_unescape($y[4]!=""?$y[4]:$y[3]),"source"=>array_map('AdminNeo\idf_unescape',$uk[0]),"target"=>array_map('AdminNeo\idf_unescape',$ll[0]),"on_delete"=>($y[6]?:"RESTRICT"),"on_update"=>($y[7]?:"RESTRICT"),];}}return$J;}function
backward_keys($Q){$H="SELECT constraint_name, table_schema, table_name, column_name, referenced_column_name
FROM information_schema.key_column_usage
WHERE table_schema = ".q(Admin::get()->getDatabase())."
AND referenced_table_schema = ".q(Admin::get()->getDatabase())."
AND referenced_table_name = ".q($Q)."
ORDER BY ordinal_position";return
get_rows($H,null,"");}function
view($A){$M=Connection::get()->getValue("SHOW CREATE VIEW ".table($A),1);$pg='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$M=preg_replace("~^$pg\\s+AS\\s+~isU","",$M);return["select"=>format_sql($M)];}function
collations(){$J=[];$H=Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("10.10")?"SELECT CHARACTER_SET_NAME AS Charset, FULL_COLLATION_NAME AS Collation, IS_DEFAULT AS `Default` FROM information_schema.COLLATION_CHARACTER_SET_APPLICABILITY":"SHOW COLLATION";foreach(get_rows($H)as$K){if($K["Default"])$J[$K["Charset"]][-1]=$K["Collation"];else$J[$K["Charset"]][]=$K["Collation"];}ksort($J);foreach($J
as$t=>$X)sort($J[$t]);return$J;}function
information_schema($g){return($g=="information_schema")||(Connection::get()->isMinVersion("5.5")&&$g=="performance_schema");}function
error(){return
h(preg_replace('~^You have an error.*syntax to use~U',"Syntax error",Connection::get()->getError()));}function
create_database($g,$Bb){return(bool)queries("CREATE DATABASE ".idf_escape($g).($Bb?" COLLATE ".q($Bb):""));}function
drop_databases($f){$J=apply_queries("DROP DATABASE",$f,'AdminNeo\idf_escape');restart_session();set_session("dbs",null);return$J;}function
rename_database($A,$Bb){$J=false;if(create_database($A,$Bb)){$S=[];$um=[];foreach(tables_list()as$Q=>$U){if($U=='VIEW')$um[]=$Q;else$S[]=$Q;}$J=(!$S&&!$um)||move_tables($S,$um,$A);drop_databases($J?[DB]:[]);}return$J;}function
auto_increment(){$Pa=" PRIMARY KEY";if($_GET["create"]!=""&&$_POST["auto_increment_col"]){foreach(indexes($_GET["create"])as$r){if(in_array($_POST["fields"][$_POST["auto_increment_col"]]["orig"],$r["columns"],true)){$Pa="";break;}if($r["type"]=="PRIMARY")$Pa=" UNIQUE";}}return" AUTO_INCREMENT$Pa";}function
alter_table($Q,$A,$k,$ce,$Kb,$ld,$Bb,$Oa,$oi){$Ga=[];foreach($k
as$j){if($j[1]){$h=$j[1][3];if(str_contains($h," GENERATED")){$j[1][3]=Connection::get()->isMariaDB()?"":$j[1][2];$j[1][2]=$h;}$Ga[]=($Q!=""?($j[0]!=""?"CHANGE ".idf_escape($j[0]):"ADD"):" ")." ".implode($j[1]).($Q!=""?$j[2]:"");}else$Ga[]="DROP ".idf_escape($j[0]);}$Ga=array_merge($Ga,$ce);$P=($Kb!==null?" COMMENT=".q($Kb):"").($ld?" ENGINE=".q($ld):"").($Bb?" COLLATE ".q($Bb):"").($Oa!=""?" AUTO_INCREMENT=$Oa":"");if($oi){$pi=[];if($oi["partition_by"]=='RANGE'||$oi["partition_by"]=='LIST'){foreach($oi["partition_names"]as$t=>$X){$Y=$oi["partition_values"][$t];$pi[]="\n  PARTITION ".idf_escape($X)." VALUES ".($oi["partition_by"]=='RANGE'?"LESS THAN":"IN").($Y!=""?" ($Y)":" MAXVALUE");}}$P
.="\nPARTITION BY {$oi["partition_by"]}({$oi["partition"]})";if($pi)$P
.=" (".implode(",",$pi)."\n)";elseif($oi["partitions"])$P
.=" PARTITIONS ".(int)$oi["partitions"];}elseif($oi===null)$P
.="\nREMOVE PARTITIONING";if($Q=="")return(bool)queries("CREATE TABLE ".table($A)." (\n".implode(",\n",$Ga)."\n)$P");if($Q!=$A)$Ga[]="RENAME TO ".table($A);if($P)$Ga[]=ltrim($P);return!$Ga||queries("ALTER TABLE ".table($Q)."\n".implode(",\n",$Ga));}function
alter_indexes($Q,$Ga){$nb=[];foreach($Ga
as$t=>$X)$nb[]=($X[2]=="DROP"?"\nDROP INDEX ".idf_escape($X[1]):"\nADD $X[0] ".($X[0]=="PRIMARY"?"KEY ":"").($X[1]!=""?idf_escape($X[1])." ":"")."(".implode(", ",$X[2]).")");return(bool)queries("ALTER TABLE ".table($Q).implode(",",$nb));}function
truncate_tables($S){return
apply_queries("TRUNCATE TABLE",$S);}function
drop_views($um){return(bool)queries("DROP VIEW ".implode(", ",array_map('AdminNeo\table',$um)));}function
drop_tables($S){return(bool)queries("DROP TABLE ".implode(", ",array_map('AdminNeo\table',$S)));}function
move_tables($S,$um,$ll){$qj=[];foreach($S
as$Q)$qj[]=table($Q)." TO ".idf_escape($ll).".".table($Q);if(!$qj||queries("RENAME TABLE ".implode(", ",$qj))){$yc=[];foreach($um
as$Q)$yc[table($Q)]=view($Q);Connection::get()->selectDatabase($ll);$g=idf_escape(DB);foreach($yc
as$A=>$sm){if(!queries("CREATE VIEW $A AS ".str_replace(" $g."," ",$sm["select"]))||!queries("DROP VIEW $g.$A"))return
false;}return
true;}return
false;}function
copy_tables($S,$um,$ll){queries("SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO'");foreach($S
as$Q){$A=($ll==DB?table("copy_$Q"):idf_escape($ll).".".table($Q));if(($_POST["overwrite"]&&!queries("\nDROP TABLE IF EXISTS $A"))||!queries("CREATE TABLE $A LIKE ".table($Q))||!queries("INSERT INTO $A SELECT * FROM ".table($Q)))return
false;foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")))as$K){$Il=$K["Trigger"];if(!queries("CREATE TRIGGER ".($ll==DB?idf_escape("copy_$Il"):idf_escape($ll).".".idf_escape($Il))." $K[Timing] $K[Event] ON $A FOR EACH ROW\n$K[Statement];"))return
false;}}foreach($um
as$Q){$A=($ll==DB?table("copy_$Q"):idf_escape($ll).".".table($Q));$sm=view($Q);if(($_POST["overwrite"]&&!queries("DROP VIEW IF EXISTS $A"))||!queries("CREATE VIEW $A AS $sm[select]"))return
false;}return
true;}function
trigger($A,$Q){if($A=="")return[];$L=get_rows("SHOW TRIGGERS WHERE `Trigger` = ".q($A));return
reset($L);}function
triggers($Q){$J=[];foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")))as$K)$J[$K["Trigger"]]=[$K["Timing"],$K["Event"]];return$J;}function
trigger_options(){return["Timing"=>["BEFORE","AFTER"],"Event"=>["INSERT","UPDATE","DELETE"],"Type"=>["FOR EACH ROW"],];}function
routine($A,$U){if($A=="")return[];$k=get_rows("SELECT
	PARAMETER_NAME field,
	DATA_TYPE type,
	REGEXP_REPLACE(DTD_IDENTIFIER, '^[^(]+\\\\(?|\\\\)$', '') length,
	REGEXP_REPLACE(DTD_IDENTIFIER, '^[^ ]+ ', '') `unsigned`,
	1 `null`,
	DTD_IDENTIFIER full_type,
	".($U=="FUNCTION"?"''":"PARAMETER_MODE")." `inout`,
	CHARACTER_SET_NAME collation
FROM information_schema.PARAMETERS
WHERE SPECIFIC_SCHEMA = DATABASE() AND ROUTINE_TYPE = '$U' AND SPECIFIC_NAME = ".q($A)."
ORDER BY ORDINAL_POSITION");$J=Connection::get()->query("SELECT
	ROUTINE_COMMENT comment,
	CONCAT(IF(IS_DETERMINISTIC = 'YES', 'DETERMINISTIC\\n', ''), IF(SQL_DATA_ACCESS != 'CONTAINS SQL', CONCAT(SQL_DATA_ACCESS, '\\n'), ''), ROUTINE_DEFINITION) definition,
	'SQL' language
FROM information_schema.ROUTINES
WHERE ROUTINE_SCHEMA = DATABASE() AND ROUTINE_TYPE = '$U' AND ROUTINE_NAME = ".q($A))->fetchAssoc();if($k&&$k[0]['field']=='')$J['returns']=array_shift($k);$J['fields']=$k;return$J;}function
routines(){return
get_rows("SELECT SPECIFIC_NAME, ROUTINE_NAME, ROUTINE_TYPE, DTD_IDENTIFIER, ROUTINE_COMMENT FROM information_schema.ROUTINES WHERE ROUTINE_SCHEMA = DATABASE()");}function
routine_languages(){return[];}function
routine_id($A,$K){return
idf_escape($A);}function
last_id($I){return
Connection::get()->getValue("SELECT LAST_INSERT_ID()");}function
explain(Connection$d,$H){return$d->query("EXPLAIN ".(Connection::get()->isMinVersion("5.7")?"":"PARTITIONS ").$H);}function
found_rows(array$R,array$Z){return$R["Engine"]=="InnoDB"&&!$Z?(int)$R["Rows"]:null;}function
format_sql($H){$pg='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$Kf='FROM|WHERE|HAVING|GROUP\s+BY|ORDER\s+BY|(NATURAL\s+)?((LEFT|RIGHT)\s+)?((INNER|OUTER|CROSS)\s+)?JOIN';$H=preg_replace("~($pg)\\s+(AS\\s+SELECT)~isU","$1 AS\nSELECT",$H);$H=preg_replace("~($pg)\\s+($Kf)~isU","$1\n$2",$H);$H=preg_replace("~($pg),~isU","$1,\n  ",$H);return$H;}function
create_sql($Q,$Oa,$Hk){$H=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if(!$Oa)$H=preg_replace('~ AUTO_INCREMENT=\d+~','',$H);return!str_contains($H,"\n")?format_sql($H):$H;}function
truncate_sql($Q){return"TRUNCATE ".table($Q);}function
create_database_sql($oc,$Hk=""){$A=idf_escape($oc);$Ib="";if(str_contains($Hk,"CREATE")&&($cc=Connection::get()->getValue("SHOW CREATE DATABASE $A",1))){set_utf8mb4($cc);if($Hk=="DROP+CREATE")$Ib="DROP DATABASE IF EXISTS $A;\n";$Ib
.="$cc;\n";}return$Ib;}function
use_sql($oc,$Hk=""){return"USE ".idf_escape($oc).";\n";}function
trigger_sql($Q){$xk="";foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")),null,"-- ")as$K)$xk
.="\nCREATE TRIGGER ".idf_escape($K["Trigger"])." $K[Timing] $K[Event] ON ".table($K["Table"])." FOR EACH ROW\n$K[Statement];;\n";return$xk;}function
show_variables(){return
get_rows("SHOW VARIABLES");}function
show_status(){return
get_rows("SHOW STATUS");}function
process_list(){return
get_rows("SHOW FULL PROCESSLIST");}function
convert_field(array$j){if(preg_match("~binary~",$j["type"]))return"HEX(".idf_escape($j["field"]).")";if($j["type"]=="bit")return"BIN(".idf_escape($j["field"])." + 0)";if($j["type"]=="vector")return(Connection::get()->isMariaDB()?"VEC_ToText":"VECTOR_TO_STRING")."(".idf_escape($j["field"]).")";if(preg_match("~geometry|point|linestring|polygon~",$j["type"]))return(Connection::get()->isMinVersion("8")?"ST_":"")."AsWKT(".idf_escape($j["field"]).")";return
null;}function
unconvert_field(array$j,$J){if(preg_match("~binary~",$j["type"]))$J="UNHEX($J)";if($j["type"]=="bit")$J="CONVERT(b$J, UNSIGNED)";if($j["type"]=="vector")$J=(Connection::get()->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."($J)";if(preg_match("~geometry|point|linestring|polygon~",$j["type"])){$Ki=(Connection::get()->isMinVersion("8")?"ST_":"");$J=$Ki."GeomFromText($J, $Ki"."SRID($j[field]))";}return$J;}function
support($Jd){return
preg_match('~^(comment|columns|copy|database|drop_col|dump|event|indexes|kill|privileges|move_col|procedure|processlist|routine|sql|status|table|trigger|variables|view'.(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.8.1":"8")?'|descidx':'').(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.2.1":"8.0.16")?'|check':'').(!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8")?'|fast_status':'').')$~',$Jd);}function
kill_process($X){return
queries("KILL ".number($X));}function
connection_id(){return"SELECT CONNECTION_ID()";}function
max_connections(){return(int)Connection::get()->getValue("SELECT @@max_connections");}}$Ci="adminneo-plugins";if(is_dir($Ci)){foreach(glob("$Ci/*.php")as$m)include_once$m;}function
get_translations($Sf){switch($Sf){case'en':$Pb=',X/&cbomt,|?z"**gW&<fbys3$
!Bh1]$S2`kS_.$dGqzQ<S3/!r4aqH.yeE7wd=x0q!;-+g"e!I^/$5YuTL%eu4iwg(}c3%4Riolj-5j=0i{eE3N59v%?}c)J`i%b-5XRg@2/NLz:/cb=D/>r5+9xN++bwA<*U@Q<[(ZNd/n>WFjIF=yjJTFrlw-HEHFx)>(2}?v8QFdAFFa>Tx!5XXM5yjw>[33=Kv-U`2)fOu:bqtRy2k?F;1Nu]9CBIvjc}Psy>mWt@6Vyfs2r8J.vFWpF%l@u/H-
{CW1b:9w/&7/<HN7-)1,fpub39QyaeqJ[U|#lv<xDaU6HEvI`o
Ir[I_TlVcm&Y>Pmvr-f@^xu?B:Up3BA#WI^7Ri8K!|_A;yG[x4$"umG"7~F>[:*$C@QH>[&`X3HGlgT1ko%1kS4^Y`61;am/wSR}_B_cjiD6$Mw,YOFvPx:PaX6vk*0/m~3W0s*b7LfbRo]51hC@Q(hL4_>k]ww&P3jwH=r`iMjfK#UbpZR`x"!]uyZ0,d:(L&4k(u^~KEj1Vfxrdqs&U)ZZm
jl&pF,Vt8Mu%J<o#`M9nia"Onhd,O-]f@0UuhgqF(isR51k0%jx3#50"@kjM`QBt15pT`3_t.=3rVxfvuRp@IbvUQ5j!5!A%=6YLt}M/ji5_3=R9InHeq#g:-ac4X"9U6h.9-Rtu/wg#[UCB`CvhFUDlGYl#)t01!(*)3BR,q(r=gi?51Ql@_>X3L/r
tsTl
=-&`7vX?[vq5;qkj~gm+2n6AmkOQKm{@7*-_6
N=;B@rJPw-eE=4:qM^KXr2a:_kp$7#qj&IGDkK%/$_jQvf.@)Zfr`o!G=jF!%iHQ^1@C5LXc;nX
bQSRyiFOHjuH.u1UhL{xxCh;2V=V"Xb9_Gvdy],ECj0[gA-18&Z8dkuEwuvCK(%]Eg+;l%0#Pv:Z,`c<b"cyW)`+dj[h1#AN&(bBF&5`}?*LguQp7*&vffeIEi3hQ;lB5uxi$YKAyT=Ix$S/a7/QusssE]AZ~IU/9%mDrW_?DJwLI]/Kan?Yz-a5UY{SCbZn.FNVIKCF5y4AL$pji,W"&D1Bjyg.O,:Ruw%jt+>8-E5aZ(Ub0w`yf)4d!MLL_yBJwxKH^"G[OsU+AdD?=O)MK+[j(S~S@NfOGeJ?zeI9DN"G);]OXB-Gr#l:EujOyOod0y.(JcW!61/2VYWU~Lt($O)(?+OOVOu02;&>GszOL$.xJ#2NhF4P{Xurc*`]NW!SiRrxVj<#^8>Y!k*<TfI9jp^N[&^PD^A6AeZpNDRA5nA465xYpF*t}H1rJ**r.]|NFwy-B0-9`0~g5@Mv7ZO&B6;d;MZqwXDcT6}f2MEoe_*-$73QH`B^Qvf</yHvCK5RS4,ps-=u;&BKcOf]+Ikq63?)Qne;R!#;V@>G?S%@Q?s3F=X@D<KQ?:(,tdw$chC;t@r
^vcXC*hu1>Cxpr
U0oV0F[,?>#$c!;Iu@K6&Y,{(R$1dX>`[RG9a5&,bVxvscZDFCD
wC"W+|ey>Mbs4zpvS0&^c!#2o:nSFntv%-NDGBiB.a8Wq<xYauyd^?DbaJPX6;&:C+v?0D3j4H`$$--{Wt#17WI_z&v!3@Wl&23B[<-0w4Q{_dX_)<6WK+HjhHK1QL:I,xmTdHvGAHX4,WuohQJhXmxdOXQq+aYM)/;oC@GryvYMP"sV+W#3`H&HQ#tGeU:w@Jl}DvqP
`kx323J`R)]ePWAma]sS)2W<Zu12^<hXU1_Xa[xK+[SJSqxb8vrM!ZQ"0K<xziiPL[W1
A.
27"QY^+
hA/,HfXb@tdFS?%7dR~U!:TwWYg,:i7w_[.K0Y:)EwO:L/vaQQ/401x4KG~%^,@NhZ5Ye-?r3]SGoBg^2I}8$>pg=D8y,MPm~)@[%nP[m-i)US]hONI2.RQ8i^.e{Q3*E3RF*1NphbZO^P=j%jTQUvi$O-%[#bPJoosM/<iRuyt=B9?
%$+N}/$7e65G-AY03=%?"NVgOk7Oh7i]dEoI44$Tf^)C>G9"B&E,&8C8$JG3Dh~j`9/<kX&lgS2w6<NmC;](LND.-2`)%UD`!
]xs%Dk]!Y[9mD"SyE@/.0!&DI$J1O4tc(Xq#0I*kFqGsswZleCi*@IuYAa;rgPNG,owVy>D#|Y#dLXCx]Mu^zYPPW8juCTN=Pdv0#[S/+9=sSlPTTNFk?tDIW?S9z+zt]I
;2Ts:Q_2iFQ*4a[#WRA=4KD,/!H$e^EaIo%4YRrP-_T|fO0
bt0(m
PKSZBd5[sAdhIZkqQ_UlW,3dm_`v[J(q!zCfg>gQH8_^A,;0g9i#2jMfmDVips+TwgqI"BvAM.4;h"vH9z"]
;1zRIVsAeSVjtL-%Och<(`#i5Qm1,+>HE!mXB?NdzI9V5jLjGvo"zV9
Qa^uzhoT!yo"p,AN)uM2l,rT65N_{;8,<8z<wl)U>h*h^)&UUN8qaS-u?BD#[Nr"t/sM5vpv-?~gIF./1[eINNW98C/I8jXqHf^g<`X>N8%Coc@K?H$M"L*f.g}@")`(WCjdN2beDJJ/L77p_RLjTF~
w-,-@rdrf8{1fh%daE-C3,,.A-/O9f<Zkj|VfMGi@H
kv;Jv{fIl-usLh9/*V)5%yKg@y6$Hr*cqWBa!H0#()<H,:sN5SEEUZT>feYPpXE:E5f<4tcBll0y_^hp.&CIjXLYJ]R/<7U6"x30HE@TG``CR{4,^B.}Eg)}B?lt0^+6LVAsre&;*5Lrrl;kpLkC=TL6R`yk?^w"a(!"6X5f*Bk)sW_If"_g-A
sVE;CPh^3?Jt[`)?~JKKfl5`OcDd`wPuZ2RDpua)~tVAJOU]ZK1wg@d:6ZyK6:r0,a&<bUBu7Hfud3W8H*X=zk%&-!<6`J<8g/E)ImQ?}%2uJ<ywtjJ%~s,u?!}
qffRI,ODPtJQdQ`ii2jZZ2YCzhJ>LLSPP_Lc{1#Bxg/h#Sq2.wV`/Ncg!pUD
arKq6rWJoY=74y4$1+Z
`KNv.X^9Z#K?-TNn[^MRQ_B.>"dz:i]HK:c~]h5rSs(s${[`Kma@R[g~65`f`>[TL2fWm5NB[iD}7S3DdSv0a8
_M{lq!1qB@]r-YDmwJ6?)>RP?P]J8yHC%';break;}return
json_decode(decompress_string($Pb),true);}function
get_plural_translation_id($t){$Di=array('Too many unsuccessful logins, try again in %d minute(s).'=>134,'%d process(es) have been killed.'=>273,'%d query(s) executed OK.'=>190,'Query executed OK, %d row(s) affected.'=>188,'%d row(s) have been imported.'=>280,'Routine has been called, %d row(s) affected.'=>224,'%d row(s)'=>187,'%d byte(s)'=>42,'%d item(s) have been affected.'=>277,);return
isset($Di[$t])?$Di[$t]:null;}$Hl=$_SESSION["translations"];$Tf=Locale::get()->getLanguage();if($_SESSION["translations_version"]!=641180328){$Hl=[];$_SESSION["translations_version"]=641180328;}if($_SESSION["translations_language"]!=$Tf){$Hl=[];$_SESSION["translations_language"]=$Tf;}if(!$Hl){$Hl=get_translations($Tf);$_SESSION["translations"]=$Hl;}Locale::get()->setTranslations($Hl);$ya=null;$jc=false;$pf=null;if(function_exists('\adminneo_instance')){$ya=\adminneo_instance();$jc=true;}elseif(file_exists("adminneo-instance.php")){$ya=include_once"adminneo-instance.php";$jc=true;}if($jc&&!$ya
instanceof
Admin&&!$ya
instanceof
Pluginer){$ya=null;$ig="href=https://github.com/adminneo-org/adminneo#advanced-customizations ".target_blank();$pf=lang(128,"<b>adminneo-instance.php</b>","<b>adminneo_instance()</b>","Admin::create()")." <a $ig>".lang(1)."</a>";}if(!$ya)$ya=Admin::create();if($pf)$ya->addError($pf);if($Ji!==null&&!isset($_GET["settings"])){$ya->getSettings()->updateParameter("lang",$Ji);redirect(remove_from_uri());}if(!defined("AdminNeo\DRIVER")){define("AdminNeo\DRIVER",null);define("AdminNeo\DIALECT",null);}define("AdminNeo\SERVER",DRIVER?$_GET[DRIVER]:null);define("AdminNeo\DB",isset($_GET["db"])?$_GET["db"]:"");define("AdminNeo\BASE_URL",preg_replace('~\?.*~','',relative_uri()));define("AdminNeo\ME",BASE_URL.'?'.(sid()?session_name()."=".urlencode(session_id()).'&':'').(SERVER!==null?DRIVER."=".urlencode(SERVER).'&':'').($_GET["ext"]?"ext=".urlencode($_GET["ext"]).'&':'').(isset($_GET["username"])?"username=".urlencode($_GET["username"]).'&':'').(DB!=""?'db='.urlencode(DB).'&'.(isset($_GET["ns"])?"ns=".urlencode($_GET["ns"])."&":""):''));define("AdminNeo\HOME_URL",BASE_URL?:".");define("AdminNeo\SERVER_HOME_URL",substr(preg_replace('~\b(username|db|ns)=[^&]*&~','',ME),0,-1)?:".");if(isset($_GET["set"])){header("Content-Type: text/javascript; charset=utf-8");if(!verify_token()){header("HTTP/1.1 403 Forbidden");exit;}if($_GET["set"]=="navigation-width"){$Fm=isset($_POST["width"])?$_POST["width"]:"";if($Fm!=""){$Fm=min(max((float)$Fm,Settings::$NavigationWidthMin),Settings::$NavigationWidthMax);Admin::get()->getSettings()->updateParameter("navigationWidth",sprintf("%.2F",$Fm));}else
Admin::get()->getSettings()->updateParameter("navigationWidth",null);}if($_GET["set"]=="export-settings")Admin::get()->getSettings()->updateParameters(["exportFormat"=>isset($_POST["format"])?$_POST["format"]:"","exportOutput"=>isset($_POST["output"])?$_POST["output"]:"",]);exit;}const
VERSION="5.7.1";function
page_header($T,$db=[]){if(!headers_sent()&&!array_sum(array_column(ob_get_status(true),"buffer_used")))ini_set("zlib.output_compression","1");page_headers();if(is_ajax()&&Admin::get()->getErrors()){page_messages();exit;}if(!ob_get_level())ob_start(null,4096);$T=strip_tags($T);$gk=$db!==false&&$db!==null&&SERVER!=""?" - ".h(Admin::get()->getServerName(SERVER)):"";$ik=strip_tags(Admin::get()->getServiceTitle());$zl=$T.$gk." - ".($ik!=""?$ik:"AdminNeo");echo'<!DOCTYPE html>
<html lang=\'',Locale::get()->getLanguage(),'\' dir=\'',lang(129),'\'>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<meta name="robots" content="noindex, nofollow">
	<meta name="viewport" content="width=device-width, initial-scale=1"/>

	<title>',$zl,'</title>

	';$Eb=validate_color_variant(Admin::get()->getConfig()->getColorVariant());echo"<link rel='stylesheet' href='",link_files("default-$Eb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("default-$Eb-dark.css",[]),"'>\n";$sl=Admin::get()->getConfig()->getTheme();list($sl,$Eb)=validate_theme($sl,$Eb);if($sl!="default"){echo"<link rel='stylesheet' href='",link_files("$sl-$Eb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("$sl-$Eb-dark.css",[]),"'>\n";}foreach(Admin::get()->getCssUrls()as$bm){if(strpos($bm,"adminneo-dark.css")===0&&!Admin::get()->isDarkModeForced())echo"<link rel='stylesheet' media='(prefers-color-scheme: dark)' href='",h($bm),"'>\n";else
echo"<link rel='stylesheet' href='",h($bm),"'>\n";}$dh=Admin::get()->getSettings()->getNavigationWidth();echo"<style id='navigation-width'>";if($dh)echo"@media screen and (min-width: 1024px) { :root { --menu-width: ",sprintf("%.2F",$dh),"rem } }";echo"</style>\n",script_src(link_files("main.js",[]));foreach(Admin::get()->getJsUrls()as$bm)echo
script_src($bm);Admin::get()->printFavicons();Admin::get()->printToHead();echo'</head>
<body class=\'',lang(129),' nojs\'>
<script',nonce(),'>
	const body = document.body;

	body.onkeydown = bodyKeydown;
	body.onclick = bodyClick;
	body.classList.replace("nojs", "js");

	const offlineMessage = \'',js_escape(lang(130)),'\';
	const thousandsSeparator = \'',js_escape(lang(105)),'\';
</script>


',"<div id='help' class='jush-".DIALECT." jsonly hidden'></div>",script("initHelpPopup();"),"<div id='content'>\n","<div class='header'>\n";if($db!==null){echo'<nav class="breadcrumbs"><ul>',"<li><a href='".h(HOME_URL)."' title='",lang(131),"'>",icon_solo("home"),"</a></li>";$ek=h(Admin::get()->getServerName(SERVER??""));if($db===false)echo"<li>$ek</li>";else{$w=substr(preg_replace('~\b(db|ns)=[^&]*&~','',ME),0,-1);echo"<li><a href='".h($w)."' accesskey='1' title='Alt+Shift+1'>$ek</a></li>";if($_GET["ns"]!=""||(DB!=""&&is_array($db)))echo'<li><a href="'.h($w."&db=".urlencode(DB).(support("scheme")?"&ns=":"")).'">'.h(DB).'</a></li>';if($db===true){if($_GET["ns"]!="")echo'<li>'.h($_GET["ns"]).'</li>';else
echo"<li>",h(DB),"</li>";}else{if($_GET["ns"]!="")echo'<li><a href="'.h(substr(ME,0,-1)).'">'.h($_GET["ns"]).'</a></li>';foreach($db
as$t=>$X){if(is_string($t)){$Ac=(is_array($X)?$X[1]:h($X));if($Ac!="")echo"<li><a href='".h(ME."$t=").urlencode(is_array($X)?$X[0]:$X)."'>$Ac</a></li>";}else
echo"<li>$X</li>\n";}}}echo"</ul></nav>";}echo"</div>\n","<h1>$T</h1>\n","<div id='ajaxstatus' class='jsonly hidden'></div>\n";restart_session();page_messages();$f=&get_session("dbs");if(DB!=""&&$f&&!in_array(DB,$f,true))$f=null;stop_session();define("AdminNeo\PAGE_HEADER",1);}function
validate_color_variant($Eb){list(,$Eb)=validate_theme("default",$Eb);return$Eb;}function
validate_theme($sl,$Eb){$tl=get_available_themes();if(!isset($tl[$sl]))$sl="default";if(!isset($tl[$sl][$Eb])){reset($tl[$sl]);$Eb=key($tl[$sl]);}return[$sl,$Eb];}function
get_available_themes(){return
array('default'=>array('blue'=>true,),);}function
page_headers(){header("Content-Type: text/html; charset=utf-8");header("Cache-Control: no-cache");header("X-XSS-Protection: 0");header("X-Content-Type-Options: nosniff");header("Referrer-Policy: origin-when-cross-origin");header("X-Frame-Options: DENY");$gc=["script-src"=>"'self' 'unsafe-inline' 'nonce-".get_nonce()."' 'strict-dynamic'","connect-src"=>"'self' https://api.github.com/repos/adminneo-org/adminneo/releases/latest","frame-src"=>"'self'","object-src"=>"'none'","base-uri"=>"'none'","form-action"=>"'self'",];Admin::get()->updateCspHeader($gc);$Fc=[];foreach($gc
as$Ec=>$vk)$Fc[]="$Ec $vk";header("Content-Security-Policy: ".implode("; ",$Fc));Admin::get()->sendHeaders();}function
get_nonce(){static$mh;if(!$mh)$mh=Random::strongKey();return$mh;}function
page_messages(){$am=preg_replace('~^[^?]*~','',$_SERVER["REQUEST_URI"]);$Jg=isset($_SESSION["messages"][$am])?$_SESSION["messages"][$am]:null;if($Jg){foreach($Jg
as$_)echo"<div class='message'>$_</div>\n",script("initToggles(qsl('.message'));");unset($_SESSION["messages"][$am]);}foreach(Admin::get()->getErrors()as$i)echo"<div class='error'>$i</div>\n";}function
page_footer($Pg=null){echo"</div>\n","<button id='navigation-button' class='button light navigation-button'>",icon_solo("menu"),icon_solo("close"),"</button>","<div id='navigation-panel' class='navigation-panel'>\n";Admin::get()->printNavigation($Pg);echo"<div class='footer'>\n","<div class='toolbox'>";if($Pg=="auth")language_select();else{$w=h(preg_replace('~\b(db|ns)=[^&]*&~',"",ME)."settings=");echo"<a class='button light' title='",lang(132),"' href='$w'>",icon_solo("settings"),"</a>";}echo"</div>";if($Pg!="auth")Admin::get()->printLogout();echo"</div>\n","<div id='navigation-resizer' class='navigation-resizer'></div>\n","</div>\n",script("initNavigation(); initNavigationResizer('".js_escape(ME)."set=navigation-width', '".get_token()."', ".Settings::$NavigationWidthMin.", ".Settings::$NavigationWidthMax.");");}function
int32($Zg){while($Zg>=2147483648)$Zg-=4294967296;while($Zg<=-2147483649)$Zg+=4294967296;return(int)$Zg;}function
long2str(array$W,$zm){$Dj='';foreach($W
as$X)$Dj
.=pack('V',$X);return$zm?substr($Dj,0,end($W)):$Dj;}function
str2long($Dj,$zm){$W=array_values(unpack('V*',str_pad($Dj,4*ceil(strlen($Dj)/4),"\0")));if($zm)$W[]=strlen($Dj);return$W;}function
xxtea_mx($Jm,$Im,$Lk,$Ef){return
int32((($Jm>>5&0x7FFFFFF)^$Im<<2)+(($Im>>3&0x1FFFFFFF)^$Jm<<4))^int32(($Lk^$Im)+($Ef^$Jm));}function
xxtea_encrypt_string($_i,$t){$t=array_values(unpack("V*",pack("H*",md5($t))));$W=str2long($_i,true);$Zg=count($W)-1;$Jm=$W[$Zg];$Im=$W[0];$Wi=floor(6+52/($Zg+1));$Lk=0;while($Wi-->0){$Lk=int32($Lk+0x9E3779B9);$Zc=$Lk>>2&3;for($di=0;$di<$Zg;$di++){$Im=$W[$di+1];$Xg=xxtea_mx($Jm,$Im,$Lk,$t[$di&3^$Zc]);$Jm=int32($W[$di]+$Xg);$W[$di]=$Jm;}$Im=$W[0];$Xg=xxtea_mx($Jm,$Im,$Lk,$t[$di&3^$Zc]);$Jm=int32($W[$Zg]+$Xg);$W[$Zg]=$Jm;}return
long2str($W,false);}function
xxtea_decrypt_string($e,$t){$t=array_values(unpack("V*",pack("H*",md5($t))));$W=str2long($e,false);$Zg=count($W)-1;$Jm=$W[$Zg];$Im=$W[0];$Wi=floor(6+52/($Zg+1));$Lk=int32($Wi*0x9E3779B9);while($Lk){$Zc=$Lk>>2&3;for($di=$Zg;$di>0;$di--){$Jm=$W[$di-1];$Xg=xxtea_mx($Jm,$Im,$Lk,$t[$di&3^$Zc]);$Im=int32($W[$di]-$Xg);$W[$di]=$Im;}$Jm=$W[$Zg];$Xg=xxtea_mx($Jm,$Im,$Lk,$t[$di&3^$Zc]);$Im=int32($W[0]-$Xg);$W[0]=$Im;$Lk=int32($Lk-0x9E3779B9);}return
long2str($W,true);}const
ENCRYPTION_GCM='aes-256-gcm';const
ENCRYPTION_CBC='aes-256-cbc';const
ENCRYPTION_TAG_LENGTH=16;const
ENCRYPTION_HMAC_LENGTH=64;function
generate_iv($u){if(function_exists('random_bytes')){try{return
random_bytes($u);}catch(Exception$Zc){}}return
openssl_random_pseudo_bytes($u);}function
hash_key($t){return
substr(hash('sha512',$t,true),0,32);}function
aes_encrypt_string($_i,$t){$Ng=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$t=hash_key($t);$Af=generate_iv(openssl_cipher_iv_length($Ng)?:16);if($Ng==ENCRYPTION_GCM)$xb=openssl_encrypt($_i,$Ng,$t,OPENSSL_RAW_DATA,$Af,$jl,"",ENCRYPTION_TAG_LENGTH);else{$xb=openssl_encrypt($_i,$Ng,$t,OPENSSL_RAW_DATA,$Af);$jl=hash_hmac("sha512",$Af.$xb,$t,true);}if($xb===false)return
false;return$Af.$jl.$xb;}function
aes_decrypt_string($e,$t){$Ng=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$Bf=openssl_cipher_iv_length($Ng)?:16;$kl=$Ng==ENCRYPTION_GCM?ENCRYPTION_TAG_LENGTH:ENCRYPTION_HMAC_LENGTH;if(strlen($e)<$Bf+$kl)return
false;$t=hash_key($t);$Af=substr($e,0,$Bf);$jl=substr($e,$Bf,$kl);$xb=substr($e,$Bf+$kl);if($Af===false||$jl===false||$xb===false)return
false;if($Ng==ENCRYPTION_GCM)return
openssl_decrypt($xb,$Ng,$t,OPENSSL_RAW_DATA,$Af,$jl);else{$Ne=hash_hmac('sha512',$Af.$xb,$t,true);if(!hash_equals($jl,$Ne))return
false;return
openssl_decrypt($xb,$Ng,$t,OPENSSL_RAW_DATA,$Af);}}function
encrypt_string($_i,$t){if($_i=="")return"";if(extension_loaded('openssl'))return
aes_encrypt_string($_i,$t);else
return
xxtea_encrypt_string($_i,$t);}function
decrypt_string($e,$t){if($e=="")return"";if(extension_loaded('openssl'))return
aes_decrypt_string($e,$t);else
return
xxtea_decrypt_string($e,$t);}$xi=[];if($_COOKIE["neo_permanent"]){foreach(explode(" ",$_COOKIE["neo_permanent"])as$X){list($t)=explode(":",$X);$xi[$t]=$X;}}function
validate_server_input(array&$xi){$N=preg_replace('~:/[-\w.][-\w.:/]*$~D',"",SERVER);if($N=="")return;if(!preg_match('~^[^:]+://~',$N))$N="https://$N";$ri=parse_url($N);if(!$ri)auth_error($xi);if(isset($ri['user'])||isset($ri['pass'])||isset($ri['query'])||isset($ri['fragment']))auth_error($xi);if(isset($ri['scheme'])&&!preg_match('~^(https?)$~i',$ri['scheme']))auth_error($xi);$Qe=$ri['host'].(isset($ri['path'])?$ri['path']:'');if(!is_server_host_valid($Qe))auth_error($xi);if(isset($ri['port'])&&($ri['port']<1024||$ri['port']>65535))auth_error($xi,lang(133));}if(!function_exists('AdminNeo\is_server_host_valid')){function
is_server_host_valid($Qe){return
strpos($Qe,'/')===false;}}function
build_http_url($N,$V,$F,$vc,$uc=null){if(!preg_match('~^(https?://)?([^:]*)(:\d+)?$~',rtrim($N,'/'),$z))return
null;return($z[1]?:"http://").($V!==""||$F!==""?urlencode($V).":".urlencode($F)."@":"").($z[2]!==""?$z[2]:$vc).(isset($z[3])?$z[3]:($uc?":$uc":""));}function
add_invalid_login(){$Xa=get_temp_dir()."/adminneo-invalid";$l=null;foreach(glob("$Xa*")?:[$Xa]as$m){$l=open_file_with_lock($m);if($l)break;}if(!$l){$l=open_file_with_lock("$Xa-".Random::strongKey());if(!$l)return;}$sf=json_decode(stream_get_contents($l),true);$vl=time();if($sf){foreach($sf
as$tf=>$X){if($X[0]<$vl)unset($sf[$tf]);}}$rf=&$sf[Admin::get()->getBruteForceKey()];if(!$rf)$rf=[$vl+30*60,0];$rf[1]++;write_and_unlock_file($l,json_encode($sf));}function
check_invalid_login(array&$xi){$Xa=get_temp_dir()."/adminneo-invalid";$sf=[];foreach(glob("$Xa*")as$m){$l=open_file_with_lock($m);if($l){$sf=json_decode(stream_get_contents($l),true);unlock_file($l);break;}}$rf=($sf?$sf[Admin::get()->getBruteForceKey()]:[]);$kh=($rf&&$rf[1]>29?$rf[0]-time():0);if($kh>0)auth_error($xi,lang(134,ceil($kh/60)));}function
connect_to_db(array&$xi){if(Admin::get()->getConfig()->hasServers()&&!Admin::get()->getConfig()->getServer(SERVER))auth_error($xi);$d=connect(true,$i);if(!$d)connection_error(nl2br(h($i)),$xi);return$d;}function
authenticate(array&$xi){$I=Admin::get()->authenticate($_GET["username"],get_password());if($I!==true)connection_error($I,$xi);}function
connection_error($i,array&$xi){$i=$i?:lang(3);if(preg_match('~^ +| +$~',get_password()))$i
.="<br>".lang(135);auth_error($xi,$i);}Admin::get()->init();$Na=isset($_POST["auth"])?$_POST["auth"]:null;if($Na){session_regenerate_id();$N=isset($Na["server"])?$Na["server"]:"";$fk=Admin::get()->getConfig()->getServer($N);$Qc=$fk?$fk->getDriver():(isset($Na["driver"])?$Na["driver"]:"");$N=$fk?$N:trim($N);$V=isset($Na["username"])?$Na["username"]:"";$F=isset($Na["password"])?$Na["password"]:"";if($fk&&$fk->hasCredentials()&&$V==""&&$F==""){$V=$fk->getUsername();$F=$fk->getPassword();}$g=$fk?$fk->getDatabase():(isset($Na["db"])?$Na["db"]:"");save_login($Qc,$N,$V,$F,$g);if($Na["permanent"]){$t=implode("-",array_map("base64_encode",[$Qc,$N,$V,$g]));$Pi=Admin::get()->getPrivateKey(true);$kd=$Pi?encrypt_string($F,$Pi):false;$xi[$t]="$t:".base64_encode($kd?:"");cookie("neo_permanent",implode(" ",$xi));}if(count($_POST)==1||DRIVER!=$Qc||SERVER!=$N||$_GET["username"]!==$V||DB!=$g)redirect(auth_url($Qc,$N,$V,$g));}elseif($_POST["logout"]&&(!$_SESSION["token"]||verify_token())){foreach(["pwds","db","dbs","queries"]as$t)set_session($t,null);unset_permanent($xi);redirect(SERVER_HOME_URL,lang(136));}elseif($xi&&!$_SESSION["pwds"]){session_regenerate_id();$Pi=Admin::get()->getPrivateKey();foreach($xi
as$t=>$X){list(,$wb)=explode(":",$X);list($Qc,$N,$V,$g)=array_map("base64_decode",explode("-",$t));$F=$Pi?decrypt_string(base64_decode($wb),$Pi):false;save_login($Qc,$N,$V,$F,$g);}}function
unset_permanent(array&$xi){foreach($xi
as$t=>$X){list($Qc,$N,$V,$g)=array_map("base64_decode",explode("-",$t));if($Qc==DRIVER&&$N==SERVER&&$V==$_GET["username"]&&$g==DB)unset($xi[$t]);}cookie("neo_permanent",implode(" ",$xi));}function
auth_error(array&$xi,$i=null){$jk=session_name();if(isset($_GET["username"])){header("HTTP/1.1 403 Forbidden");if(($_COOKIE[$jk]||$_GET[$jk])&&!$_SESSION["token"])$i=lang(137);else{restart_session();add_invalid_login();$F=get_password();if($F!==null){if($F===false)$i=lang(138);delete_login(DRIVER,SERVER,$_GET["username"]);}unset_permanent($xi);}}if(!$_COOKIE[$jk]&&$_GET[$jk]&&ini_bool("session.use_only_cookies"))$i=lang(139);if(!$i)$i=lang(3);Admin::get()->addError($i);print_login_page();}function
print_login_page(){$gi=session_get_cookie_params();cookie("neo_key",($_COOKIE["neo_key"]?:Random::strongKey()),$gi["lifetime"]);if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);page_header(lang(31),null);echo"<form action='' method='post'>\n","<div>";if(print_hidden_fields($_POST,["auth"]))echo"<p class='message'>".lang(140)."\n";echo"</div>\n";Admin::get()->printLoginForm();echo"</form>\n";page_footer("auth");exit;}if(isset($_GET["username"])&&!DRIVER)print_login_page();if(isset($_GET["username"])&&!defined('AdminNeo\DRIVER_EXTENSION')){Admin::get()->addError(lang(141,implode(", ",Drivers::getExtensions(DRIVER))));unset($_SESSION["pwds"][DRIVER]);unset_permanent($xi);page_header(lang(142),false);page_footer("auth");exit;}if(!isset($_GET["username"])||get_password()===null)print_login_page();validate_server_input($xi);check_invalid_login($xi);Admin::get()->getConfig()->applyServer(SERVER);$d=connect_to_db($xi);authenticate($xi);create_driver($d);if($_POST["logout"]&&$_SESSION["token"]&&!verify_token()){Admin::get()->addError(lang(143));page_header(lang(6));page_footer("db");exit;}if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);stop_session(true);if($Na&&$_POST["token"])$_POST["token"]=get_token();if($_POST){if(!verify_token()){$hf="max_input_vars";$Bg=ini_get($hf);if(extension_loaded("suhosin")){foreach(["suhosin.request.max_vars","suhosin.post.max_vars"]as$t){$X=ini_get($t);if($X&&(!$Bg||$X<$Bg)){$hf=$t;$Bg=$X;}}}if(!$_POST["token"]&&$Bg)Admin::get()->addError(lang(144,"'$hf'"));else
Admin::get()->addError(lang(143).' '.lang(145));$_POST=[];}}elseif($_SERVER["REQUEST_METHOD"]=="POST"){$i=lang(146,"'post_max_size'");if(isset($_GET["sql"]))$i
.=' '.lang(147);Admin::get()->addError($i);}if(isset($_GET["settings"])){$O=Admin::get()->getSettings();$mk=array_merge(Admin::get()->getSettingsRows(1),Admin::get()->getSettingsRows(2),Admin::get()->getSettingsRows(3));if($_POST){$gi=[];foreach($mk
as$t=>$K){if(isset($_POST[$t])){$dm=$_POST[$t]===""||(is_array($_POST[$t])&&in_array("",$_POST[$t]));$gi[$t]=(!$dm?$_POST[$t]:null);}}$O->updateParameters($gi);redirect(remove_from_uri());}$T=lang(132);page_header($T,[$T]);echo"<form id='settings' action='' method='post'>\n","<table class='box'>\n";foreach($mk
as$K)echo$K;echo"</table>\n","<p>","<input type='submit' value='".lang(113),"' class='button default hidden'>",input_token(),"</p>\n","</form>\n",script("initSettingsForm();");page_footer();exit;}if(isset($_GET["status"]))$_GET["variables"]=$_GET["status"];if(isset($_GET["import"]))$_GET["sql"]=$_GET["import"];if(!(DB!=""?Connection::get()->selectDatabase(DB):isset($_GET["sql"])||isset($_GET["dump"])||isset($_GET["database"])||isset($_GET["processlist"])||isset($_GET["privileges"])||isset($_GET["user"])||isset($_GET["variables"])||$_GET["script"]=="connect"||$_GET["script"]=="kill")){if(DB!=""||$_GET["refresh"]){restart_session();set_session("dbs",null);}if(DB!=""){Admin::get()->addError(lang(148));header("HTTP/1.1 404 Not Found");page_header(lang(30).": ".h(DB),true);}else{if($_POST["db"])queries_redirect(substr(ME,0,-1),lang(149),drop_databases($_POST["db"]));$T=h(Drivers::get(DRIVER).": ".Admin::get()->getServerName(SERVER));page_header($T,false);$jg=['privileges'=>[lang(72),"users"],'processlist'=>[lang(150),"list"],'variables'=>[lang(151),"variable"],'status'=>[lang(152),"status"],];$kg="";foreach($jg
as$t=>$X){if(support($t))$kg
.="<a href='".h(ME)."$t='>".icon($X[1])."$X[0]</a>";}if($kg)echo"<p class='links top-links'>$kg</p>\n";echo"<p>".lang(153,Drivers::get(DRIVER),"<b>".h(Connection::get()->getVersion())."</b>","<b>".DRIVER_EXTENSION."</b>")."\n","<p>".lang(154,"<b>".h(logged_user())."</b>")."\n";$f=Admin::get()->getDatabases();if($f){$Nj=support("scheme");$Da=collations();echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr>".(support("database")?"<td>":"")."<th>".lang(30).(get_session("dbs")!==null?" - <a href='".h(ME)."refresh=1'>".lang(155)."</a>":"")."<td>".lang(45)."<td>".lang(156)."<td>".lang(157)." - <a href='".h(ME)."dbsize=1'>".lang(158)."</a>".script("qsl('a').onclick = partial(ajaxSetHtml, '".js_escape(ME)."script=connect');","")."</thead>\n","<tbody>\n";$f=($_GET["dbsize"]?count_tables($f):array_flip($f));foreach($f
as$g=>$S){$zj=h(ME)."db=".urlencode($g);$q=h("Db-".$g);echo"<tr>".(support("database")?"<td class='actions'>".checkbox("db[]",$g,in_array($g,(array)$_POST["db"]),"","","",$q):""),"<th><a href='$zj' id='$q'>".h($g)."</a>";$Bb=h(db_collation($g,$Da));echo"<td>".(support("database")?"<a href='$zj".($Nj?"&amp;ns=":"")."&amp;database=' title='".lang(69)."'>$Bb</a>":$Bb),"<td align='right'><a href='$zj&amp;schema=' id='tables-".h($g)."' title='".lang(71)."'>".($_GET["dbsize"]?$S:"?")."</a>","<td align='right' id='size-".h($g)."'>".($_GET["dbsize"]?db_size($g):"?"),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: partialArg(tableClick, true)});"),"</table>\n","</div>\n";if(support("database"))echo"<div class='table-footer'><div class='field-sets'>\n","<fieldset><legend>",lang(159)," <span id='selected'></span></legend><div class='fieldset-content'>\n",input_hidden("all"),script("qsl('input').onclick = function () { selectCount('selected', formChecked(this, /^db/)); };"),"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(),"\n","</div></fieldset>\n","</div></div>\n",script("initTableFooter()");echo"</div>\n",input_token(),"</form>\n",script("tableCheck();");}}echo'<p class="links"><a href="'.h(ME).'database=">'.icon("database-add").lang(75)."</a>\n";page_footer("db");exit;}if(isset($_GET["select"])&&($_POST["edit"]||$_POST["clone"])&&!$_POST["save"])$_GET["edit"]=$_GET["select"];if(isset($_GET["callf"]))$_GET["call"]=$_GET["callf"];if(isset($_GET["function"]))$_GET["procedure"]=$_GET["function"];if(isset($_GET["download"])){$a=$_GET["download"];$k=fields($a);header("Content-Type: application/octet-stream");header("Content-Disposition: attachment; filename=".friendly_url("$a-".implode("_",$_GET["where"])).".".friendly_url($_GET["field"]));$M=[idf_escape($_GET["field"])];$I=Driver::get()->select($a,$M,[where($_GET,$k)],$M);$K=($I?$I->fetchRow():[]);echo
Connection::get()->formatValue($K[0],$k[$_GET["field"]]);exit;}elseif(isset($_GET["table"])){$a=$_GET["table"];$k=fields($a);if(!$k)Admin::get()->addError(error()?:lang(78));$R=table_status1($a,true);$A=Admin::get()->getTableName($R);$yj=[];foreach($k
as$t=>$j)$yj+=$j["privileges"];$T=$k&&is_view($R)?$R['Engine']=='materialized view'?lang(161):lang(162):lang(8);$Zk=$A!=""?$A:h($a);page_header("$T: $Zk",[$Zk]);$nf=null;if(isset($yj["insert"])||!support("table"))$nf=[];Admin::get()->printTableMenu($R,$nf);$ff=[];if(!preg_match("~sqlite|mssql|pgsql~",DIALECT)&&isset($R["Engine"]))$ff[]=lang(163).": ".h($R["Engine"]);if(isset($R["Collation"]))$ff[]=lang(45).": ".h($R["Collation"]);if($ff)echo"<p>",implode(", ",$ff),"</p>";if($k)Admin::get()->printTableStructure($k);$Kb=$R["Comment"];if($Kb!="")echo"<p class='keep-lines'>",lang(46),": ",Admin::get()->formatComment($Kb),"</p>\n";if(!is_view($R))$bd='<p class="links"><a href="'.h(ME).'create='.urlencode($a).'">'.icon("edit").lang(35)."</a>\n";elseif(support("view"))$bd='<p class="links"><a href="'.h(ME).'view='.urlencode($a).'">'.icon("edit").lang(36)."</a>\n";else$bd="";if($ff||$k||$Kb!="")echo$bd;$hi=Driver::get()->getParentTables($a);if($hi){echo"<h2>".lang(164)."</h2>\n";Admin::get()->printRelatedTables($hi);}if(Driver::get()->getPartitionBy()&&str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned")){$qi=Driver::get()->getPartitionsInfo($a);if($qi){echo"<h2 id='partitions'>".lang(49)."</h2>\n";Admin::get()->printTablePartitions($qi);if(DIALECT!="pgsql")echo$bd;}}$gf=Driver::get()->getInheritedTables($a);if($gf){echo"<h2 id='inherited-by'>".lang(165)."</h2>\n";Admin::get()->printRelatedTables($gf);}if(support("indexes")&&Driver::get()->supportsIndex($R)){echo"<h2 id='indexes'>".lang(166)."</h2>\n";$s=indexes($a);if($s)Admin::get()->printTableIndexes($s,$R);echo'<p class="links"><a href="'.h(ME).'indexes='.urlencode($a).'">'.icon("edit").lang(167)."</a>\n";}if(!is_view($R)){if(fk_support($R)){echo"<h2 id='foreign-keys'>".lang(90)."</h2>\n";$ee=foreign_keys($a);if($ee){echo"<table>\n","<thead><tr><th>".lang(168)."<td>".lang(169)."<td>".lang(93)."<td>".lang(92)."<td></thead>\n";foreach($ee
as$A=>$n)echo"<tr title='".h($A)."'>","<th><i>".implode("</i>, <i>",array_map('AdminNeo\h',$n["source"]))."</i>","<td><a href='".h($n["db"]!=""?preg_replace('~db=[^&]*~',"db=".urlencode($n["db"]),ME):($n["ns"]!=""?preg_replace('~ns=[^&]*~',"ns=".urlencode($n["ns"]),ME):ME))."table=".urlencode($n["table"])."'>".($n["db"]!=""&&$n["db"]!=DB?"<b>".h($n["db"])."</b>.":"").($n["ns"]!=""&&$n["ns"]!=$_GET["ns"]?"<b>".h($n["ns"])."</b>.":"").h($n["table"])."</a>","(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$n["target"]))."</i>)","<td>".h($n["on_delete"]),"<td>".h($n["on_update"]),'<td><a href="'.h(ME.'foreign='.urlencode($a).'&name='.urlencode($A)).'">'.lang(170).'</a>',"\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'foreign='.urlencode($a).'">'.icon("add").lang(171)."</a>\n";}if(support("check")){echo"<h2 id='checks'>".lang(172)."</h2>\n";$rb=Driver::get()->checkConstraints($a);if($rb){echo"<table cellspacing='0'>\n";foreach($rb
as$t=>$X)echo"<tr title='".h($t)."'>","<td><code class='jush-".DIALECT."'>".h($X),"<td><a href='".h(ME.'check='.urlencode($a).'&name='.urlencode($t))."'>".lang(170)."</a>","\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'check='.urlencode($a).'">'.icon("add").lang(173)."</a>\n";}}if(support(is_view($R)?"view_trigger":"trigger")){echo"<h2 id='triggers'>".lang(174)."</h2>\n";$Kl=triggers($a);if($Kl){echo"<table>\n";foreach($Kl
as$t=>$X)echo"<tr><td>".h($X[0])."<td>".h($X[1])."<th>".h($t)."<td><a href='".h(ME.'trigger='.urlencode($a).'&name='.urlencode($t))."'>".lang(170)."</a>\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'trigger='.urlencode($a).'">'.icon("add").lang(175)."</a>\n";}}elseif(isset($_GET["schema"])){$yl=h(": ".DB.($_GET["ns"]?".$_GET[ns]":""));page_header(lang(71).$yl,[lang(71)]);$bl=[];$cl=[];$Nd=[];$pa=($_GET["schema"]?:$_COOKIE["neo_schema-".str_replace(".","_",DB)]);preg_match_all('~([^:]+):([-0-9.]+)x([-0-9.]+)(_|$)~',$pa,$z,PREG_SET_ORDER);foreach($z
as$p=>$y){$bl[$y[1]]=[(float)$y[2],(float)$y[3]];$cl[]="\n\t'".js_escape($y[1])."': [ $y[2], $y[3] ]";}$Cl=0;$Wa=-1;$Lj=[];$lj=[];$Zf=[];$Ea=Driver::get()->getAllFields();foreach(table_status('',true)as$Q=>$R){if(is_view($R))continue;$G=0;$Lj[$Q]["fields"]=[];foreach(isset($Ea[$Q])?$Ea[$Q]:[]as$j){$G+=1.25;$Nd[$Q][$j["field"]]=$G;$Lj[$Q]["fields"][$j["field"]]=$j;}$Lj[$Q]["pos"]=(isset($bl[$Q])?$bl[$Q]:[$Cl,0]);foreach(Admin::get()->getForeignKeys($Q)as$X){if(!$X["db"]){$Xf=$Wa;if((isset($bl[$Q][1])?$bl[$Q][1]:0)||(isset($bl[$X["table"]][1])?$bl[$X["table"]][1]:0))$Xf=min(floatval(isset($bl[$Q][1])?$bl[$Q][1]:0),floatval(isset($bl[$X["table"]][1])?$bl[$X["table"]][1]:0))-1;else$Wa-=.1;while($Zf[(string)$Xf])$Xf-=.0001;$Lj[$Q]["references"][$X["table"]][(string)$Xf]=[$X["source"],$X["target"]];$lj[$X["table"]][$Q][(string)$Xf]=$X["target"];$Zf[(string)$Xf]=true;}}$Cl=max($Cl,$Lj[$Q]["pos"][0]+2.5+$G);}echo"<div id='schema' style='height: {$Cl}em;'>\n","<script",nonce(),">\n","gid('schema').onselectstart = () => false;\n","const tablePos = {",implode(",",$cl),"\n};\n","const em = gid('schema').offsetHeight / $Cl;\n","document.onmousemove = schemaMousemove;\n","document.onmouseup = partialArg(schemaMouseup, '",js_escape(DB),"');\n","</script>\n";foreach($Lj
as$A=>$Q){echo"<div class='table' style='top: ".$Q["pos"][0]."em; left: ".$Q["pos"][1]."em;'>",'<a href="'.h(ME).'table='.urlencode($A).'"><b>'.h($A)."</b></a>",script("qsl('div').onmousedown = schemaMousedown;");foreach($Q["fields"]as$j){$X='<span '.type_class($j["type"]).' title="'.h($j["type"].($j["length"]?"($j[length])":"").($j["null"]?" NULL":'')).'">'.h($j["field"]).'</span>';echo"<br>".($j["primary"]?"<i>$X</i>":$X);}foreach((array)$Q["references"]as$ml=>$nj){foreach($nj
as$Xf=>$hj){$Yf=$Xf-(isset($bl[$A][1])?$bl[$A][1]:0);$p=0;foreach($hj[0]as$uk){echo"\n<div class='references' title='",h($ml),"' id='refs$Xf-$p' style='left: {$Yf}em; top: ",$Nd[$A][$uk],"em; padding-top: .5em;'>","<div style='border-top: 1px solid Gray; width: ".(-$Yf)."em;'></div>","</div>";$p++;}}}foreach((array)$lj[$A]as$ml=>$nj){foreach($nj
as$Xf=>$c){$Yf=$Xf-(isset($bl[$A][1])?$bl[$A][1]:0);$p=0;foreach($c
as$ll){echo"\n<div class='references' title='",h($ml),"' id='refd$Xf-$p' style='left: {$Yf}em; top: ".$Nd[$A][$ll]."em; height: 1.25em;'>","<svg style='width: 1em; height: 1em; float: right;' viewBox='0 0 22 22' fill='currentColor'><path d='M11,19l10,-8l-10,-8l0,16Z'/></svg>","<div style='height: .5em; border-bottom: 1px solid Gray; width: ".(-$Yf)."em;'></div>","</div>";$p++;}}}echo"\n</div>\n";}foreach($Lj
as$A=>$Q){foreach((array)$Q["references"]as$ml=>$nj){if($Lj[$ml]){foreach($nj
as$Xf=>$hj){$Og=$Cl;$zg=-10;foreach($hj[0]as$t=>$uk){$Fi=$Q["pos"][0]+$Nd[$A][$uk];$Gi=$Lj[$ml]["pos"][0]+$Nd[$ml][$hj[1][$t]];$Og=min($Og,$Fi,$Gi);$zg=max($zg,$Fi,$Gi);}echo"<div class='references' id='refl$Xf' style='left: $Xf"."em; top: $Og"."em; padding: .5em 0;'><div style='border-right: 1px solid Gray; margin-top: 1px; height: ".($zg-$Og)."em;'></div></div>\n";}}}}echo"</div>\n","<p class='links'>","<a href='",(ME."schema=".urlencode($pa)),"' id='schema-link'>",lang(176),"</a>","</p>\n";}elseif(isset($_GET["dump"])){$a=$_GET["dump"];$O=Admin::get()->getSettings();if($_POST){$O->updateParameters(["dumpFormat"=>$_POST["format"],"dumpDbStyle"=>$_POST["db_style"],"dumpTypes"=>isset($_POST["types"])?$_POST["types"]:(support("type")?"":null),"dumpRoutines"=>isset($_POST["routines"])?$_POST["routines"]:(support("routine")?"":null),"dumpEvents"=>isset($_POST["events"])?$_POST["events"]:(support("event")?"":null),"dumpTableStyle"=>$_POST["table_style"],"dumpAutoIncrement"=>isset($_POST["auto_increment"])?$_POST["auto_increment"]:"","dumpTriggers"=>isset($_POST["triggers"])?$_POST["triggers"]:(support("trigger")?"":null),"dumpDataStyle"=>$_POST["data_style"],"dumpOutput"=>$_POST["output"],]);if(DB!="")$f=[DB];else{$f=isset($_POST["databases"])?$_POST["databases"]:[];if(is_string($f))$f=explode("\n",rtrim(str_replace("\r","",$f),"\n"));}$Mj=isset($_POST["schemas"])?$_POST["schemas"]:[];$S=array_flip(isset($_POST["tables"])?$_POST["tables"]:[])+array_flip(isset($_POST["data"])?$_POST["data"]:[]);if(count($S)==1)$Ue=key($S);elseif(count($Mj)==1)$Ue=$Mj[0];elseif(count($f)==1)$Ue=$f[0];else$Ue=Admin::get()->getServerName(SERVER,true,"server");$Bd=dump_headers($Ue,DB==""||$_GET["ns"]===""||count($S)>1);$yf=preg_match('~sql~',$_POST["format"]);$mc=$yf&&$_POST["data_style"]&&!$_POST["table_style"]&&DIALECT!="sql";if($yf){echo"-- AdminNeo ".VERSION." ".Drivers::get(DRIVER)." ".Connection::get()->getVersion()." dump\n\n";if(DIALECT=="sql"){echo"SET NAMES utf8;
SET time_zone = '+00:00';
SET foreign_key_checks = 0;
".($_POST["data_style"]?"SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO';
":"")."
";Connection::get()->query("SET time_zone = '+00:00'");Connection::get()->query("SET sql_mode = ''");}}$Hk=$_POST["db_style"];foreach($f
as$g){Admin::get()->dumpDatabase($g);if(Connection::get()->selectDatabase($g)){if($yf){if($Hk)echo
create_database_sql($g,$Hk),use_sql($g,$Hk)."\n";$ai="";if($_POST["types"]){foreach(types()as$q=>$U){$od=type_values($q);if($od)$ai
.=($Hk!='DROP+CREATE'?"DROP TYPE IF EXISTS ".idf_escape($U).";;\n":"")."CREATE TYPE ".idf_escape($U)." AS ENUM ($od);\n\n";else$ai
.="-- Could not export type $U\n\n";}}if($_POST["routines"]){foreach(routines()as$K){$A=$K["ROUTINE_NAME"];$_j=$K["ROUTINE_TYPE"];$cc=create_routine($_j,["name"=>$A]+routine($K["SPECIFIC_NAME"],$_j));set_utf8mb4($cc);$ai
.=($Hk!='DROP+CREATE'?"DROP $_j IF EXISTS ".idf_escape($A).";;\n":"")."$cc;\n\n";}}if($_POST["events"]){foreach(get_rows("SHOW EVENTS",null,"-- ")as$K){$cc=remove_definer(Connection::get()->getValue("SHOW CREATE EVENT ".idf_escape($K["Name"]),3));set_utf8mb4($cc);$ai
.=($Hk!='DROP+CREATE'?"DROP EVENT IF EXISTS ".idf_escape($K["Name"]).";;\n":"")."$cc;;\n\n";}}echo($ai&&DIALECT=='sql'?"DELIMITER ;;\n\n$ai"."DELIMITER ;\n\n":$ai);}if($_POST["table_style"]||$_POST["data_style"]){foreach(($_GET["ns"]===""?(array)$_POST["schemas"]:(DB!=""||!support("scheme")?[""]:Admin::get()->getSchemas(true)))as$Lj){if($Lj!="")set_schema($Lj);$hl=table_status('',true);$al=array_keys($hl);$Gc=false;if($mc&&$al){$mj=[];foreach($al
as$A){if(!is_view($hl[$A])&&(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]))){foreach(foreign_keys($A)as$n)$mj[$A][]=$n["table"];}}$Qh=dump_table_order($al,$mj);if($Qh)$al=$Qh;else$Gc=function_exists('AdminNeo\foreign_key_checks_sql');}if($Gc)echo
foreign_key_checks_sql(false)."\n";$um=[];foreach($al
as$A){$R=$hl[$A];$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));$e=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]));if($Q||$e){$_l=null;if($Bd=="tar"){$_l=new
TmpFile();ob_start([$_l,'write'],1e5);}$dc=($Q?$_POST["table_style"]:"");Admin::get()->dumpTable($A,$dc,(is_view($R)?2:0));if(is_view($R)&&$Bd!="tar")$um[]=$A;elseif($e){$k=fields($A);Admin::get()->dumpData($A,$_POST["data_style"],"SELECT *".convert_fields($k,$k)." FROM ".table($A));if($yf&&!$dc&&$_POST["auto_increment"]&&function_exists('AdminNeo\restart_sequences_sql'))echo"\n".restart_sequences_sql($A);}if($yf&&$_POST["triggers"]&&$Q&&($Kl=trigger_sql($A)))echo"\nDELIMITER ;;\n$Kl\nDELIMITER ;\n";if($Bd=="tar"){ob_end_flush();tar_file((DB!=""?"":"$g/")."$A.csv",$_l);}elseif($yf)echo"\n";}}if($Gc)echo
foreign_key_checks_sql(true)."\n";if($_POST["table_style"]&&function_exists('AdminNeo\foreign_keys_sql')){foreach($hl
as$A=>$R){$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));if($Q&&!is_view($R))echo
foreign_keys_sql($A);}}foreach($um
as$sm)Admin::get()->dumpTable($sm,$_POST["table_style"],1);if($Bd=="tar")echo
pack("x512");}}}}if($yf)echo"-- ".gmdate("Y-m-d H:i:s e")."\n";exit;}$A=DB!=""?h(DB):h(Admin::get()->getServerName(SERVER));page_header(lang(74).": $A",($_GET["export"]!=""?["table"=>$_GET["export"]]:[lang(74)]));echo"<form action='' method='post'>\n","<table class='box'>\n";$qc=['','USE','DROP+CREATE','CREATE'];$el=['','DROP+CREATE','CREATE'];$nc=['','TRUNCATE+INSERT','INSERT'];if(DIALECT=="sql")$nc[]='INSERT+UPDATE';echo"<tr><th>",lang(177),"</th><td>",html_radios("format",Admin::get()->getDumpFormats(),$O->getParameter("dumpFormat","sql")),"</td></tr>\n";if(DIALECT!="sqlite"){echo"<tr><th id='label-db'>",lang(30),"</th>","<td>",html_select('db_style',$qc,$O->getParameter("dumpDbStyle",DB==""?"CREATE":""),"","label-db"),"<span class='labels'>";if(support("routine"))echo
checkbox("routines",1,$O->getParameter("dumpRoutines",$_GET["dump"]==""?"1":""),lang(178));if(support("event"))echo
checkbox("events",1,$O->getParameter("dumpEvents",$_GET["dump"]==""?"1":""),lang(179));echo"</span></td></tr>";}echo"<tr><th id='label-tables'>",lang(156),"</th><td>",html_select('table_style',$el,$O->getParameter("dumpTableStyle","DROP+CREATE"),"","label-tables")," <span class='labels'>",checkbox("auto_increment",1,$O->getParameter("dumpAutoIncrement"),lang(47));if(support("trigger"))echo
checkbox("triggers",1,$O->getParameter("dumpTriggers","1"),lang(174));echo"</span></td></tr>","<tr><th id='label-data'>",lang(180),"</th><td>",html_select("data_style",$nc,$O->getParameter("dumpDataStyle","INSERT"),"","label-data"),"</td></tr>","<tr><th>",lang(181),"</th><td>",html_radios("output",Admin::get()->getDumpOutputs(),$O->getParameter("dumpOutput","file")),"</td></tr>\n","</table>\n","<p>","<input type='submit' class='button default' value='",lang(74),"'>",input_token(),"</p>\n","<table>\n",script("qsl('table').onclick = dumpClick;");$Li=[];if(DB!=""&&$_GET["ns"]===""){echo"<thead><tr><th>","<label class='block'><input type='checkbox' id='check-schemas' checked class='jsonly'>".lang(182)."</label>".script("gid('check-schemas').onclick = partial(formCheck, /^schemas\\[/);",""),"</thead>\n";foreach(Admin::get()->getSchemas()as$Lj)echo"<tr><td>".checkbox("schemas[]",$Lj,true,$Lj,"","block")."\n";}elseif(DB!=""){$tb=($a!=""?"":" checked");echo"<thead><tr>","<th><label class='block'><input type='checkbox' id='check-tables'$tb class='jsonly'>".lang(8)."</label>".script("gid('check-tables').onclick = partial(formCheck, /^tables\\[/);",""),"<th class='right'><label class='block'>".lang(180)."<input type='checkbox' id='check-data'$tb class='jsonly'></label>".script("gid('check-data').onclick = partial(formCheck, /^data\\[/);",""),"</thead>\n";$um="";$gl=tables_list();foreach($gl
as$A=>$U){$Ki=preg_replace('~_.*~','',$A);$tb=($a==""||$a==(substr($a,-1)=="%"?"$Ki%":$A));$Oi="<tr><td>".checkbox("tables[]",$A,$tb,$A,"","block");if($U!==null&&!preg_match('~table~i',$U))$um
.="$Oi\n";else
echo"$Oi<td class='right'><label class='block'><span id='Rows-".h($A)."'></span>".checkbox("data[]",$A,$tb)."</label>\n";$Li[$Ki]++;}echo$um;if($gl)echo
script("ajaxSetHtml('".js_escape(ME)."script=db');");}else{$f=Admin::get()->getDatabases();echo"<thead><tr><th>","<label class='block'>".($f?"<input type='checkbox' id='check-databases'".($a==""?" checked":"")." class='jsonly'>".script("gid('check-databases').onclick = partial(formCheck, /^databases\\[/);",""):"").lang(30)."</label>","</thead>\n";if($f){foreach($f
as$g){if(!information_schema($g)){$Ki=preg_replace('~_.*~','',$g);echo"<tr><td>".checkbox("databases[]",$g,$a==""||$a=="$Ki%",$g,"","block")."\n";$Li[$Ki]++;}}}else
echo"<tr><td><textarea name='databases' rows='10' cols='20'></textarea>";}echo"</table>\n","</form>\n";$jg=[];foreach($Li
as$t=>$X){if($t!=""&&$X>1)$jg[]="<a href='".h(ME)."dump=".urlencode("$t%")."'>".icon("check").h($t)."*</a>";}if($jg)echo"<p class='links'>",implode("",$jg),"</p>\n";}elseif(isset($_GET["privileges"])){$yl=DB!=""?h(": ".DB):"";page_header(lang(72).$yl,[lang(72)]);echo'<p class="links top-links"><a href="',h(ME),'user=">',icon("user-add"),lang(183),"</a></p>\n";$I=Connection::get()->query("SELECT User, Host FROM mysql.".(DB==""?"user":"db WHERE ".q(DB)." LIKE Db")." ORDER BY Host, User");$ue=$I;if(!$I)$I=Connection::get()->query("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', 1) AS User, SUBSTRING_INDEX(CURRENT_USER, '@', -1) AS Host");echo"<form action=''>\n";hidden_fields_get();echo
input_hidden("db",DB);if(!$ue)echo
input_hidden("grant");echo"\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr><th>".lang(28)."<th>".lang(5)."<th></thead>\n";while($K=$I->fetchAssoc())echo'<tr><td>'.h($K["User"])."<td>".h($K["Host"]).'<td><a href="'.h(ME.'user='.urlencode($K["User"]).'&host='.urlencode($K["Host"])).'">'.lang(38)."</a>\n";if(!$ue||DB!="")echo"<tr><td><input class='input' name='user' autocapitalize='off'><td><input class='input' name='host' value='localhost' autocapitalize='off'><td><input type='submit' class='button' value='".lang(38)."'>\n";echo"</table>\n","</div>\n","</form>\n";}elseif(isset($_GET["sql"])){$O=Admin::get()->getSettings();if($_POST["export"]){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers("sql");Admin::get()->dumpTable("","");Admin::get()->dumpData("","table",$_POST["query"]);exit;}restart_session();$Me=&get_session("queries");$Le=&$Me[DB];if($_POST["clear"]){$Le=[];redirect(remove_from_uri("history"));}stop_session();$T=isset($_GET["import"])?lang(73):lang(40);page_header($T,[$T]);$gg="--".(DIALECT=="sql"?" ":"");if($_POST){$le=false;if(!isset($_GET["import"]))$H=$_POST["query"];elseif($_POST["webfile"]){$Ye=Admin::get()->getImportFilePath();if($Ye){if(file_exists($Ye))$le=fopen($Ye,"rb");elseif(file_exists("$Ye.gz"))$le=fopen("compress.zlib://$Ye.gz","rb");}$H=$le?fread($le,1e6):false;}else$H=get_file("sql_file",true,";");if(is_string($H)){if(($Eg=ini_bytes("memory_limit"))!="-1")ini_set("memory_limit",max($Eg,strval(2*strlen($H)+memory_get_usage()+8e6)));if($H!=""&&strlen($H)<1e6){$Wi=$H.(preg_match("~;[ \t\r\n]*\$~",$H)?"":";");if(!$Le||first(end($Le))!=$Wi){restart_session();$Le[]=[$Wi,time()];set_session("queries",$Me);stop_session();}}$wk="(?:\\s|/\\*[\s\S]*?\\*/|(?:#|$gg)[^\n]*\n?|--\r?\n)";$zc=";";$_c=1;$sh=0;$hd=true;$Ub=connect();if($Ub&&DB!=""){$Ub->selectDatabase(DB);if($_GET["ns"]!="")set_schema($_GET["ns"],$Ub);}$Jb=0;$qd=[];$ii='[\'"'.(DIALECT=="sql"?'`#':(DIALECT=="sqlite"?'`[':(DIALECT=="mssql"?'[':''))).']|/\*|'.$gg.'|$'.(DIALECT=="pgsql"?'|\$([a-zA-Z]\w*)?\$':'');$Dl=microtime(true);$Yc=Admin::get()->getDumpFormats();unset($Yc["sql"]);while($H!=""){if(!$sh&&preg_match("~^$wk*+DELIMITER\\s+(\\S+)~i",$H,$y)){$zc=preg_quote($y[1]);$_c=strlen($y[1]);$he=Admin::get()->formatSqlCommandQuery(trim($y[0]));if($he!="")echo"<pre><code class='jush-".DIALECT."'>$he</code></pre>\n";$H=substr($H,strlen($y[0]));}elseif(!$sh&&DIALECT=="pgsql"&&preg_match("~^($wk*+COPY\\s+)[^;]+\\s+FROM\\s+stdin;~i",$H,$y)){$zc="\n\\\\\\.\r?\n";$_c=3;$sh=strlen($y[0]);}else{preg_match("($zc\\s*|$ii)",$H,$y,PREG_OFFSET_CAPTURE,$sh);list($je,$G)=$y[0];if(!$je&&$le&&!feof($le))$H
.=fread($le,1e5);else{if(!$je&&rtrim($H)=="")break;$sh=$G+strlen($je);if($je&&!preg_match("(^$zc)",$je)){$ib=Driver::get()->hasCStyleEscapes()||(DIALECT=="pgsql"&&($G>0&&strtolower($H[$G-1])=="e"));$vi='(';if($je=='/*')$vi
.='\*/';elseif($je=='[')$vi
.=']';elseif(preg_match("~^$gg|^#~",$je))$vi
.="\n";else$vi
.=preg_quote($je).($ib?"|\\\\.":"");$vi
.='|$)s';while(preg_match($vi,$H,$y,PREG_OFFSET_CAPTURE,$sh)){$Dj=$y[0][0];if(!$Dj&&$le&&!feof($le))$H
.=fread($le,1e5);else{$sh=$y[0][1]+strlen($Dj);if(!isset($Dj[0])||$Dj[0]!="\\")break;}}}else{$hd=false;$Wi=substr($H,0,$G+$_c);$Jb++;$Oi="<pre id='sql-$Jb'><code class='jush-".DIALECT."'>".Admin::get()->formatSqlCommandQuery(trim($Wi))."</code></pre>\n";if(DIALECT=="sqlite"&&preg_match("~^$wk*+(ATTACH|VACUUM\\b.*\\bINTO)\\b~is",$Wi,$y)!==0){echo$Oi,"<p class='error'>".lang(184,preg_match('~ATTACH~i',$y[1])?'ATTACH':'VACUUM INTO')."\n";$qd[]=" <a href='#sql-$Jb'>$Jb</a>";if($_POST["error_stops"])break;}else{if(!$_POST["only_errors"]){echo$Oi;ob_flush();flush();}$Ak=microtime(true);if(Connection::get()->multiQuery($Wi)&&is_object($Ub)&&preg_match("~^$wk*+USE\\b~i",$Wi))$Ub->query($Wi);do{$I=Connection::get()->storeResult();if(Connection::get()->getError()){echo($_POST["only_errors"]?$Oi:""),"<p class='error'>",lang(185),(!empty(Connection::get()->getErrno())?" (".Connection::get()->getErrno().")":""),": ",error()."</p>\n";$qd[]=" <a href='#sql-$Jb'>$Jb</a>";if($_POST["error_stops"])break
2;}else{$vl=" <span class='time'>(".format_time($Ak).")</span>";$cd=(strlen($Wi)<1000?" <a href='".h(ME)."sql=".urlencode(trim($Wi))."'>".icon("edit").lang(38)."</a>":"");$aj=Connection::get()->getQueryInfo();$za=Connection::get()->getAffectedRows();$_m=($_POST["only_errors"]?null:Driver::get()->warnings());$Bm="warnings-$Jb";$Cm=$_m?"<a href='#$Bm' class='toggle'>".lang(39).icon_chevron_down()."</a>":null;$yd=$Th=null;$zd="explain-$Jb";$_d=false;$Ad="export-$Jb";$v=0;if(is_object($I)){if(!$_POST["only_errors"])echo"<div class='table-result'>\n";$v=(int)$_POST["limit"];$Th=print_select_result($I,$Ub,[],$v);if(!$_POST["only_errors"]){echo"<p class='links'>";$ph=$I->getRowsCount();echo($ph?($v&&$ph>$v?lang(186,$v):"").lang(187,$ph):""),$vl,$cd,$Cm;if($Ub&&preg_match("~^($wk|\\()*+SELECT\\b~i",$Wi)&&($yd=explain($Ub,$Wi)))echo"<a href='#$zd' class='toggle'>Explain".icon_chevron_down()."</a>";$_d=true;echo"<a href='#$Ad' class='toggle'>".lang(74).icon_chevron_down()."</a>","</p>\n";}}else{if(preg_match("~^$wk*+(CREATE|DROP|ALTER)$wk++(DATABASE|SCHEMA)\\b~i",$Wi)){restart_session();set_session("dbs",null);stop_session();}if(!$_POST["only_errors"]){echo"<p class='message' title='".h($aj)."'>",lang(188,$za),"$vl $cd";if($Cm)echo", $Cm";echo"</p>\n";}}if(!$_POST["only_errors"])echo
script("initToggles(qsl('p'));");if($_m)echo"<div id='$Bm' class='hidden'>\n$_m</div>\n";if($yd){echo"<div id='$zd' class='hidden explain'>\n";print_select_result($yd,$Ub,$Th);echo"</div>\n";}if($_d){echo"<form id='$Ad' action='' method='post' class='hidden'><p>\n",html_select("format",$Yc,$O->getParameter("exportFormat")),html_select("output",Admin::get()->getDumpOutputs(),$O->getParameter("exportOutput"))." ",input_hidden("query",$Wi),input_token()," <input type='submit' class='button' name='export' value='".lang(74)."'>";if(!$v)echo
script("qsl('input').onclick = partial(sqlExport, '".js_escape(ME)."set=export-settings');","");echo"</p></form>\n";}if(is_object($I)&&!$_POST["only_errors"])echo"</div>\n";}$Ak=microtime(true);}while(Connection::get()->nextResult());}$H=substr($H,$sh);$sh=0;}}}}if($hd)echo"<p class='message'>".lang(189)."\n";elseif($_POST["only_errors"]){$vh=$Jb-count($qd);echo"<p class='".($vh?"message":"error")."'>".lang(190,$Jb-count($qd))," <span class='time'>(".format_time($Dl).")</span>\n";}elseif($qd&&$Jb>1)echo"<p class='error'>".lang(185).": ".implode("",$qd)."\n";}else
echo"<p class='error'>".upload_error($H)."\n";}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";if(!isset($_GET["import"])){$Wi=$_GET["sql"];if($_POST)$Wi=$_POST["query"];elseif($_GET["history"]=="all")$Wi=$Le;elseif($_GET["history"]!="")$Wi=$Le[$_GET["history"]][0];echo"<p>";textarea("query",$Wi,20);echo
script(($_POST?"":"qs('textarea').focus();\n")."gid('form').onsubmit = partial(sqlSubmit, gid('form'), '".js_escape(remove_from_uri("sql|limit|error_stops|only_errors|history"))."');"),"</p>","<p><input type='submit' class='button default' value='".lang(191)."' title='Ctrl+Enter'>",lang(192).": <input type='number' name='limit' class='input size' value='".h($_POST?$_POST["limit"]:$_GET["limit"])."'>\n";}else{echo"<div class='field-sets'>\n","<fieldset><legend>".lang(193)."</legend><div class='fieldset-content'>";$Be=(extension_loaded("zlib")?"[.gz]":"");if(ini_bool("file_uploads"))echo"SQL$Be (&lt; ".ini_get("upload_max_filesize")."B): <input type='file' name='sql_file[]' multiple>","<input type='submit' class='button default' value='".lang(191)."'>",file_upload_form_script("form","sql_file[]");else
echo
lang(194);echo"</div></fieldset>\n";$Ye=Admin::get()->getImportFilePath();if($Ye)echo"<fieldset><legend>".lang(195)."</legend><div class='fieldset-content'>",lang(196,"<code>".h($Ye)."$Be</code>")," <input type='submit' class='button default' name='webfile' value='".lang(197)."'>","</div></fieldset>\n";echo"</div>\n","<p>";}echo
checkbox("error_stops",1,($_POST?$_POST["error_stops"]:isset($_GET["import"])||$_GET["error_stops"]),lang(198)),checkbox("only_errors",1,($_POST?$_POST["only_errors"]:isset($_GET["import"])||$_GET["only_errors"]),lang(199)),input_token(),"</p>\n";if(!isset($_GET["import"]))Admin::get()->printAfterSqlCommand();if(!isset($_GET["import"])&&$Le){echo"<div class='field-sets'>\n";print_fieldset_start("history",lang(200),"history",$_GET["history"]!="");for($X=end($Le);$X;$X=prev($Le)){$t=key($Le);list($Wi,$vl,$gd)=$X;echo" <pre><code class='jush-".DIALECT."'>",truncate_utf8(ltrim(str_replace("\n"," ",str_replace("\r","",preg_replace("~^(#|$gg).*~m",'',$Wi))))),"</code></pre>",'<p class="links">',"<a href='".h(ME."sql=&history=$t")."'>".icon("edit").lang(38)."</a>"," <span class='time' title='".@date('Y-m-d',$vl)."'>".@date("H:i:s",$vl).($gd?" ($gd)":"")."</span>","</p>";}echo"<p><input type='submit' class='button' name='clear' value='".lang(201)."'>\n","<a href='",h(ME."sql=&history=all")."' class='button light'>",icon("edit"),lang(202),"</a></p>\n";print_fieldset_end("history");echo"</div>\n";}echo"</form>\n";}elseif(isset($_GET["edit"])){$a=$_GET["edit"];$k=fields($a);$Z=(isset($_GET["select"])?($_POST["check"]&&count($_POST["check"])==1?where_check($_POST["check"][0],$k):""):where($_GET,$k));$Zl=(isset($_GET["select"])?$_POST["edit"]:$Z);foreach($k
as$A=>$j){if((!$Zl&&!isset($j["privileges"]["insert"]))||Admin::get()->getFieldName($j)=="")unset($k[$A]);}if($_POST&&!isset($_GET["select"])){$x=$_POST["referer"];if($_POST["insert"])$x=($Zl?null:$_SERVER["REQUEST_URI"]);elseif(!preg_match('~^.+&select=.+$~',$x))$x=ME."select=".urlencode($a);$s=indexes($a);$Tl=unique_array(isset($_GET["where"])?$_GET["where"]:[],$s);$bj="\nWHERE $Z";if(isset($_POST["delete"]))queries_redirect($x,lang(203),(bool)Driver::get()->delete($a,$bj,$Tl?0:1));else{$kk=[];foreach($k
as$A=>$j){$X=process_input($j);if($X!==false&&$X!==null)$kk[idf_escape($A)]=$X;}if($Zl){if(!$kk)redirect($x);queries_redirect($x,lang(204),(bool)Driver::get()->update($a,$kk,$bj,$Tl?0:1));if(is_ajax()){page_headers();page_messages();exit;}}else{$I=Driver::get()->insert($a,$kk);$Vf=($I?last_id($I):0);queries_redirect($x,lang(205,($Vf?" $Vf":"")),(bool)$I);}}}$K=null;if($Z){$M=[];foreach($k
as$A=>$j){if(isset($j["privileges"]["select"])){$La=($_POST["clone"]&&$j["auto_increment"]?"''":convert_field($j));$M[]=($La?"$La AS ":"").idf_escape($A);}}$K=[];if(!support("table"))$M=["*"];if($M){$I=Driver::get()->select($a,$M,[$Z],$M,[],(isset($_GET["select"])?2:1));if(!$I)Admin::get()->addError(error());else{$K=$I->fetchAssoc();if(!$K)$K=false;}if(isset($_GET["select"])&&(!$K||$I->fetchAssoc()))$K=null;}}if(!support("table")&&!$k){if(!$Z){$I=Driver::get()->select($a,["*"],[],["*"]);$K=($I?$I->fetchAssoc():false);if(!$K)$K=[Driver::get()->primary=>""];}if($K){foreach($K
as$t=>$X){if(!$Z)$K[$t]=null;$k[$t]=["field"=>$t,"null"=>($t!=Driver::get()->primary),"auto_increment"=>($t==Driver::get()->primary)];}}}if(isset($_POST["save"])?$_POST["save"]:false){$Hi=[];foreach((isset($_POST["fields"])?$_POST["fields"]:[])as$t=>$X)$Hi[bracket_escape($t,true)]=$X;$K=$Hi+($K?:[]);}if($_POST["edit"]){$ed=array_filter($k,function($j){return!(isset($j["generated"])?$j["generated"]:null);});}else$ed=$k;edit_form($a,$ed,$K,$Zl);}elseif(isset($_GET["create"])){$a=$_GET["create"];$mi=Driver::get()->getPartitionBy();$qi=$mi?Driver::get()->getPartitionsInfo($a):[];$jj=referencable_primary($a);$ee=[];foreach($jj
as$Zk=>$j)$ee[str_replace("`","``",$Zk)."`".str_replace("`","``",$j["field"])]=$Zk;$Wh=[];$R=[];if($a!=""){$Wh=fields($a);$R=table_status1($a);if(count($R)<2)Admin::get()->addError(lang(78));}$K=$_POST;$K["Comment"]=normalize_newlines($K["Comment"]);$K["fields"]=(array)$K["fields"];if($K["auto_increment_col"])$K["fields"][$K["auto_increment_col"]]["auto_increment"]=true;if($_POST&&!Admin::get()->getErrors())Admin::get()->getSettings()->updateParameter("commentsOpened",isset($_POST["comments"])?$_POST["comments"]:null);if($_POST&&!process_fields($K["fields"])&&!Admin::get()->getErrors()){if($_POST["drop"])queries_redirect(substr(ME,0,-1),lang(206),drop_tables([$a]));else{$k=[];$Ea=[];$em=false;$ce=[];$Vh=reset($Wh);$Aa=" FIRST";foreach($K["fields"]as$t=>$j){$n=$ee[$j["type"]];$Nl=($n!==null?$jj[$n]:$j);if($j["field"]!=""){if(!$j["generated"])$j["default"]=null;$Ui=process_field($j,$Nl);$Ea[]=[$j["orig"],$Ui,$Aa];if(!$Vh||$Ui!==process_field($Vh,$Vh)){$k[]=[$j["orig"],$Ui,$Aa];if($j["orig"]!=""||$Aa)$em=true;}if($n!==null)$ce[idf_escape($j["field"])]=($a!=""&&DIALECT!="sqlite"?"ADD":" ").format_foreign_key(['table'=>$ee[$j["type"]],'source'=>[$j["field"]],'target'=>[$Nl["field"]],'on_delete'=>$j["on_delete"],]);$Aa=" AFTER ".idf_escape($j["field"]);}elseif($j["orig"]!=""){$em=true;$k[]=[$j["orig"]];}if($j["orig"]!=""){$Vh=next($Wh);if(!$Vh)$Aa="";}}$oi=[];if(in_array($K["partition_by"],$mi)){foreach($K
as$t=>$X){if(preg_match('~^partition~',$t))$oi[$t]=$X;}foreach($oi["partition_names"]as$t=>$A){if($A===""){unset($oi["partition_names"][$t]);unset($oi["partition_values"][$t]);}}$oi["partition_names"]=array_values($oi["partition_names"]);$oi["partition_values"]=array_values($oi["partition_values"]);if($oi==$qi)$oi=[];}elseif(str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned"))$oi=null;$_=lang(207);if($a==""){cookie("neo_engine",isset($K["Engine"])?$K["Engine"]:"");$_=lang(208);}$A=trim($K["name"]);queries_redirect(ME.(support("table")?"table=":"select=").urlencode($A),$_,alter_table($a,$A,(DIALECT=="sqlite"&&($em||$ce)?$Ea:$k),$ce,($K["Comment"]!=$R["Comment"]?$K["Comment"]:null),($K["Engine"]&&$K["Engine"]!=$R["Engine"]?$K["Engine"]:""),($K["Collation"]&&$K["Collation"]!=$R["Collation"]?$K["Collation"]:""),($K["Auto_increment"]!=""?number($K["Auto_increment"]):""),$oi));}}if($a!="")page_header(lang(35).": ".h($a),["table"=>$a,lang(35)]);else
page_header(lang(77),[lang(77)]);if(!$_POST){$Pl=Driver::get()->getTypes();$K=["Engine"=>$_COOKIE["neo_engine"],"fields"=>[["field"=>"","type"=>(isset($Pl["int"])?"int":(isset($Pl["integer"])?"integer":"")),"on_update"=>""]],"partition_names"=>[""],];if($a!=""){$K=$R;$K["name"]=$a;$K["fields"]=[];if(!$_GET["auto_increment"])$K["Auto_increment"]="";foreach($Wh
as$j){$j["generated"]=$j["generated"]?:(isset($j["default"])?"DEFAULT":"");$K["fields"][]=$j;}if($mi){$K+=$qi;$K["partition_names"][]="";$K["partition_values"][]="";}}}$Gf=[];if($K["Collation"])$Gf[$K["Collation"]]=true;foreach($K["fields"]as$j){if($j["collation"])$Gf[$j["collation"]]=true;}$Cb=Admin::get()->getCollations(array_keys($Gf));$md=Driver::get()->engines();foreach($md
as$ld){if(!strcasecmp($ld,$K["Engine"])){$K["Engine"]=$ld;break;}}echo"<form action='' method='post' id='form'>\n";if(support("columns")||$a==""){echo"<p>",lang(209),": ","<input class='input' name='name' data-maxlength='64' value='",h($K["name"]),"' autocapitalize='off'",(($a==""&&!$_POST)?" autofocus":""),">";if($md)echo" ",html_select("Engine",[""=>"(".lang(210).")"]+$md,$K["Engine"]),help_script_command("value",true);if($Cb&&!preg_match("~sqlite|mssql~",DIALECT))echo" ",html_select("Collation",[""=>"(".lang(91).")"]+$Cb,$K["Collation"]);echo" <input type='submit' class='button default' value='",lang(113),"'>","</p>";}if(support("columns")&&($a==""||!Driver::get()->isPartition($a))){echo"<div class='scrollable'>\n","<table id='edit-fields' class='nowrap'>\n";edit_fields($K["fields"],$Cb,"TABLE",$ee);echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>",lang(47),": ","<input type='number' class='input size' name='Auto_increment' size='6' value='",h($K["Auto_increment"]),"'>";$Nb=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Lb=$Nb?"":"hidden";if(support("comment")){echo
checkbox("comments",1,$Nb,lang(46),"editingCommentsClick(this, ".(support("move_col")?7:6).");","jsonly")," ";if(preg_match('~\n~',$K["Comment"]))echo"<textarea name='Comment' rows='2' cols='20'",($Lb?" class='$Lb'":""),">",h($K["Comment"]),"</textarea>";else
echo"<input name='Comment' value='",h($K["Comment"]),"' data-maxlength='",(Connection::get()->isMinVersion("5.5")?2048:60),"' class='input $Lb'>";}echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(113),"'>";}elseif($a!="")echo"<p>";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$a)),"</p>\n";if($mi&&(DIALECT=="sql"||$a=="")){echo"<div class='field-sets'>\n";$ni=preg_match('~RANGE|LIST~',$K["partition_by"]);print_fieldset_start("partition",lang(212),"split",(bool)$K["partition_by"]);echo"<p>",html_select("partition_by",array_merge([""],$mi),$K["partition_by"]),help_script_command("value.replace(/./, 'PARTITION BY \$&')",true),script("qsl('select').onchange = partitionByChange;"),"(<input class='input' name='partition' value='",h($K["partition"]),"'>) ",lang(49),": ","<input type='number' name='partitions' class='input size ",($ni||!$K["partition_by"]?"hidden":""),"' value='",h($K["partitions"]),"'>","</p>\n","<table id='partition-table'",($ni?"":" class='hidden'"),">\n","<thead><tr><th>",lang(213),"</th><th>",lang(51),"</th></tr></thead>\n";foreach($K["partition_names"]as$t=>$X){echo"<tr>","<td><input class='input' name='partition_names[]' value='",h($X),"' autocapitalize='off'>";if($t==count($K["partition_names"])-1)echo
script("qsl('input').oninput = partitionNameChange;");echo"</td>","<td><input class='input' name='partition_values[]' value='",h(isset($K["partition_values"][$t])?$K["partition_values"][$t]:""),"'></td>","</tr>\n";}echo"</table>\n","</p>\n";print_fieldset_end("partition");echo"</div>\n";}echo
input_token(),"</form>\n";}elseif(isset($_GET["indexes"])){$a=$_GET["indexes"];$ef=["PRIMARY","UNIQUE","INDEX"];$R=table_status1($a,true);$cf=Driver::get()->getIndexAlgorithms($R);$d=Connection::get();$sg=$d->isMariaDB();if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($sg?"10.0.5":"5.6")?'|InnoDB':'').'~i',$R["Engine"]))$ef[]="FULLTEXT";if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($sg?"10.2.2":"5.7")?'|InnoDB':'').'~i',$R["Engine"]))$ef[]="SPATIAL";if($sg&&$d->isMinVersion("11.7")&&preg_match('~MyISAM|InnoDB~i',$R["Engine"]))$ef[]="VECTOR";$s=indexes($a);$k=fields($a);$Ni=[];if(DIALECT=="mongo"){$Ni=$s["_id_"];unset($ef[0]);unset($s["_id_"]);}$K=$_POST;if($K){$O=Admin::get()->getSettings();if($O->getParameter("indexOptions")!==null)$O->updateParameter("indexOptions",null);}if($_POST&&!$_POST["add"]&&!$_POST["drop_col"]){$Ga=[];foreach($K["indexes"]as$r){$A=$r["name"];if(in_array($r["type"],$ef)){$c=[];$dg=[];$Cc=[];$Hh=[];$bf=$cf?(in_array($r["algorithm"],$cf)?$r["algorithm"]:first($cf)):"";$df=(support("partial_indexes")?$r["partial"]:"");$kk=[];ksort($r["columns"]);foreach($r["columns"]as$t=>$b){if($b!=""){$u=isset($r["lengths"][$t])?$r["lengths"][$t]:null;$Ac=isset($r["descs"][$t])?$r["descs"][$t]:null;$Gh=isset($r["opclasses"][$t])?$r["opclasses"][$t]:null;$kk[]=($k[$b]?idf_escape($b):$b).($u?"(".(+$u).")":"").($Gh!=""?" ".idf_escape($Gh):"").($Ac?" DESC":"");$c[]=$b;$dg[]=($u?:null);$Cc[]=$Ac;$Hh[]="$Gh";}}$xd=$s[$A];if($xd){ksort($xd["columns"]);ksort($xd["lengths"]);ksort($xd["descs"]);if($r["type"]==$xd["type"]&&array_values($xd["columns"])===$c&&(!$xd["lengths"]||array_values($xd["lengths"])===$dg)&&array_values($xd["descs"])===$Cc&&(!$xd["opclasses"]||array_values($xd["opclasses"])===$Hh)&&(!$cf||$xd["algorithm"]===$bf)&&$xd["partial"]==$df){unset($s[$A]);continue;}}if($c)$Ga[]=[$r["type"],$A,$kk,$bf,$df];}}foreach($s
as$A=>$xd)$Ga[]=[$xd["type"],$A,"DROP"];if(!$Ga)redirect(ME."table=".urlencode($a));queries_redirect(ME."table=".urlencode($a),lang(214),alter_indexes($a,$Ga));}page_header(lang(167),["table"=>$a,lang(167)],h($a));$Pd=array_keys($k);if($_POST["add"]){foreach($K["indexes"]as$t=>$r){if($r["columns"][count($r["columns"])]!="")$K["indexes"][$t]["columns"][]="";}$r=end($K["indexes"]);if($r["type"]||array_filter($r["columns"],'strlen'))$K["indexes"][]=["columns"=>[1=>""]];}if(!$K){foreach($s
as$t=>$r){$s[$t]["name"]=$t;$s[$t]["columns"][]="";}$s[]=["columns"=>[1=>""]];$K["indexes"]=$s;}$dg=(DIALECT=="sql"||DIALECT=="mssql");$Hh=Driver::get()->getIndexOpclasses();if($_POST)$ok=$_POST["options"];else{$ok=false;foreach($s
as$r){if(array_filter(isset($r["lengths"])?$r["lengths"]:[])||array_filter(isset($r["descs"])?$r["descs"]:[])||array_filter(isset($r["opclasses"])?$r["opclasses"]:[])||(isset($r["partial"])?$r["partial"]:"")!=""){$ok=true;break;}}}echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th id='label-type'>",lang(215),"</th>";$Mh="class='idxopts".($ok?"":" hidden")."'";if(count($cf)>1)echo"<th id='label-method' $Mh>",lang(216),doc_link(['sql'=>'create-index.html#create-index-storage-engine-index-types','mariadb'=>'ha-and-performance/optimization-and-tuning/optimization-and-indexes/storage-engine-index-types',]),"</th>";echo"<th><input type='submit' hidden>",lang(52).($dg?"<span $Mh> (".lang(53).")</span>":"");if($dg||support("descidx"))echo
checkbox("options",1,$ok,lang(97),"indexOptionsShow(this.checked)","jsonly")."\n";echo"</th>","<th id='label-name'>",lang(217),"</th>";if(support("partial_indexes"))echo"<th id='label-condition' $Mh>",lang(54),"</th>";echo"<th>","<button name='add[0]' value='1' title='",lang(98),"' class='button light hidden'>",icon_solo("add"),"</button>","</th>","</tr></thead>\n";if($Ni){echo"<tr><td>PRIMARY<td>";foreach($Ni["columns"]as$b)echo
select_input(" disabled",$Pd,$b),"<label><input type='checkbox' disabled>".lang(62)."</label> ";echo"<td><td>\n";}$Cf=1;foreach($K["indexes"]as$r){if(!$_POST["drop_col"]||$Cf!=key($_POST["drop_col"])){echo"<tr><td>",html_select("indexes[$Cf][type]",[-1=>""]+$ef,$r["type"],($Cf==count($K["indexes"])?"indexesAddRow.call(this);":""),"label-type"),"</td>";if(count($cf)>1)echo"<td $Mh>",html_select("indexes[$Cf][algorithm]",array_merge([""],$cf),$r['algorithm'],"label-method"),"</td>";echo"<td>";ksort($r["columns"]);$p=1;foreach($r["columns"]as$t=>$b){echo"<span>".select_input(" name='indexes[$Cf][columns][$p]' title='".lang(43)."'",($k&&($b==""||$k[$b])?array_combine($Pd,$Pd):[]),$b,"partial(".($p==count($r["columns"])?"indexesAddColumn":"indexesChangeColumn").", '".js_escape(DIALECT=="sql"?"":$_GET["indexes"]."_")."')"),"<span $Mh>";if($dg)echo"<input type='number' name='indexes[$Cf][lengths][$p]' class='input size' value='".(h(isset($r["lengths"][$t])?$r["lengths"][$t]:"")),"' title='".lang(96),"'>";if($Hh){$Gh=isset($r["opclasses"][$t])?$r["opclasses"][$t]:"";echo
html_select("indexes[$Cf][opclasses][$p]",[""=>"(".lang(218).")"]+array_combine($Hh,$Hh)+($Gh!=""?[$Gh=>$Gh]:[]),$Gh),'';}if(support("descidx"))echo
checkbox("indexes[$Cf][descs][$p]",1,isset($r["descs"][$t])?$r["descs"][$t]:false,lang(62));echo"<br></span></span>";$p++;}echo"</td>","<td><input name='indexes[$Cf][name]' value='",h($r["name"]),"' class='input' autocapitalize='off' aria-labelledby='label-name'></td>\n";if(support("partial_indexes"))echo"<td $Mh><input name='indexes[$Cf][partial]' value='".h($r["partial"])."' autocapitalize='off' aria-labelledby='label-condition'>\n";echo"<td>","<button name='drop_col[$Cf]' value='1' title='",lang(58),"' class='button light'>",icon_solo("remove"),"</button>",script("qsl('button').onclick = onRemoveIndexRowClick;"),"</td>\n";}$Cf++;}echo"</table>\n","</div>\n","<p>","<input type='submit' class='button default' value='",lang(113),"'>",input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["database"])){$K=$_POST;if($_POST&&!isset($_POST["add_x"])){$A=trim($K["name"]);if($_POST["drop"]){$_GET["db"]="";queries_redirect(remove_from_uri("db|database"),lang(219),drop_databases([DB]));}elseif(DB!==$A){if(DB!=""){$_GET["db"]=$A;queries_redirect(preg_replace('~\bdb=[^&]*&~','',ME)."db=".urlencode($A),lang(220),rename_database($A,$K["collation"]));}else{$f=explode("\n",str_replace("\r","",$A));$Jk=true;$Uf="";foreach($f
as$g){if(count($f)==1||$g!=""){if(!create_database($g,$K["collation"]))$Jk=false;$Uf=$g;}}restart_session();set_session("dbs",null);queries_redirect(ME."db=".urlencode($Uf),lang(221),$Jk);}}else{if(!$K["collation"])redirect(substr(ME,0,-1));query_redirect("ALTER DATABASE ".idf_escape($A).(preg_match('~^[a-z0-9_]+$~i',$K["collation"])?" COLLATE $K[collation]":""),substr(ME,0,-1),lang(222));}}if(DB!="")page_header(lang(69).": ".h(DB),[lang(69)]);else
page_header(lang(75),[lang(75)]);$A=DB;if($_POST)$A=$K["name"];elseif(DB!="")$K["collation"]=db_collation(DB,collations());elseif(DIALECT=="sql"){foreach(get_vals("SHOW GRANTS")as$ue){if(preg_match('~ ON (`(([^\\\\`]|``|\\\\.)*)%`\.\*)?~',$ue,$y)&&$y[1]){$A=stripcslashes(idf_unescape("`$y[2]`"));break;}}}$Cb=Admin::get()->getCollations($K["collation"]?[$K["collation"]]:[]);echo"<form action='' method='post'>\n","<p>";if($_POST["add_x"]||strpos($A,"\n"))echo"<textarea id='name' name='name' rows='10' cols='40'>",h($A),"</textarea><br>\n";else
echo"<input class='input' name='name' id='name' value='",h($A),"' data-maxlength='64' autocapitalize='off' autofocus>\n";if($Cb)echo
html_select("collation",[""=>"(".lang(91).")"]+$Cb,$K["collation"]),doc_link(['sql'=>"charset-charsets.html",'mariadb'=>"reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations",]),"\n";echo"<input type='submit' class='button default' value='",lang(113),"'>\n";if(DB!="")echo"<input type='submit' class='button' name='drop' value='".lang(160)."'>".confirm(lang(211,DB))."\n";elseif(!$_POST["add_x"]&&$_GET["db"]=="")echo"<button name='add_x' value='1' title='",lang(98),"' class='button light'>",icon_solo("add"),"</button>\n";echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["call"])){$oa=$_GET["name"]?:$_GET["call"];page_header(lang(223).": ".h($oa),[lang(223)]);$_j=routine($_GET["call"],(isset($_GET["callf"])?"FUNCTION":"PROCEDURE"));$Ze=[];$ai=[];foreach($_j["fields"]as$p=>$j){if(substr($j["inout"],-3)=="OUT"&&DIALECT=='sql')$ai[$p]="@".idf_escape($j["field"])." AS ".idf_escape($j["field"]);if(!$j["inout"]||substr($j["inout"],0,2)=="IN")$Ze[]=$p;}if($_POST){$kb=[];foreach($_j["fields"]as$t=>$j){$X="";if(in_array($t,$Ze)){$X=process_input($j);if($X===false)$X="''";if(isset($ai[$t]))Connection::get()->query("SET @".idf_escape($j["field"])." = $X");}if(isset($ai[$t]))$kb[]="@".idf_escape($j["field"]);elseif(in_array($t,$Ze))$kb[]=$X;}$H=(isset($_GET["callf"])?"SELECT ":"CALL ").($_j["returns"]&&$_j["returns"]["type"]=="record"?"* FROM ":"").table($oa)."(".implode(", ",$kb).")";$Ak=microtime(true);$I=Connection::get()->multiQuery($H);$za=Connection::get()->getAffectedRows();echo
Admin::get()->formatSelectQuery($H,$Ak,!$I);if(!$I)echo"<p class='error'>".error()."\n";else{$Ub=connect();if($Ub)$Ub->selectDatabase(DB);do{$I=Connection::get()->storeResult();if(is_object($I))print_select_result($I,$Ub);else
echo"<p class='message'>".lang(224,$za)." <span class='time'>".@date("H:i:s")."</span>\n";}while(Connection::get()->nextResult());if($ai)print_select_result(Connection::get()->query("SELECT ".implode(", ",$ai)));}}echo"<form action='' method='post'>\n";if($Ze){echo"<table class='box'>\n";foreach($Ze
as$t){$j=$_j["fields"][$t];$A=$j["field"];echo"<tr><th>".Admin::get()->getFieldName($j);$Y=isset($_POST["fields"][$A])?$_POST["fields"][$A]:"";if($Y!=""){if($j["type"]=="set")$Y=implode(",",$Y);}input($j,$Y,(string)(isset($_POST["function"][$A])?$_POST["function"][$A]:""));echo"\n";}echo"</table>\n";}echo"<p>\n","<input type='submit' class='button' value='",lang(223),"'>\n",input_token(),"</p>\n","</form>\n";$Kb=$_j["comment"];if($Kb!==null&&$Kb!==""){$Kb=h(trim($_j["comment"],"\n"));if(preg_match('~^ +~',$Kb,$z)){preg_match_all("~^($z[0]|$)~m",$Kb,$hg);if(count($hg[0])==substr_count($Kb,"\n"))$Kb=preg_replace("~^($z[0])~m","",$Kb);}$Kb=preg_replace('~(^|[^\n]\n)(Description|Parameters|Example)\n~',"$1\n<strong>$2</strong>\n",$Kb);echo"<pre class='comment'>$Kb</pre>\n";}}elseif(isset($_GET["foreign"])){$a=$_GET["foreign"];$A=$_GET["name"];$K=$_POST;if($_POST&&!$_POST["add"]&&!$_POST["change"]&&!$_POST["change-js"]){if(!$_POST["drop"]){$K["source"]=array_filter($K["source"],'strlen');ksort($K["source"]);$ll=[];foreach($K["source"]as$t=>$X)$ll[$t]=$K["target"][$t];$K["target"]=$ll;}if(DIALECT=="sqlite")$I=recreate_table($a,$a,[],[],[" $A"=>($K["drop"]?"":" ".format_foreign_key($K))]);else{$Ga="ALTER TABLE ".table($a);$I=($A==""||queries("$Ga DROP ".(DIALECT=="sql"?"FOREIGN KEY ":"CONSTRAINT ").idf_escape($A)));if(!$K["drop"])$I=queries("$Ga ADD".format_foreign_key($K));}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(225):($A!=""?lang(226):lang(227))),(bool)$I);if(!$K["drop"])Admin::get()->addError(lang(228));}page_header(lang(229).": ".h($a),["table"=>$a,lang(229)]);if($_POST){ksort($K["source"]);if($_POST["change"]||$_POST["change-js"])$K["target"]=[];else$K["source"][]="";}elseif($A!=""){$ee=foreign_keys($a);$K=$ee[$A];$K["source"][]="";}else{$K["table"]=$a;$K["source"]=[""];}echo"<form action='' method='post'>\n";$uk=array_keys(fields($a));if($K["db"]!="")Connection::get()->selectDatabase($K["db"]);if($K["ns"]!=""){$Xh=get_schema();set_schema($K["ns"]);}$ij=array_keys(array_filter(table_status('',true),'AdminNeo\fk_support'));$ll=array_keys(fields(in_array($K["table"],$ij)?$K["table"]:reset($ij)));$Ch="this.form['change-js'].value = '1'; this.form.submit();";echo"<p>","<span id='label-table'>",lang(230),":</span> ",html_select("table",$ij,$K["table"],$Ch,"label-table");if(DIALECT!="sqlite"){$rc=[];foreach(Admin::get()->getDatabases()as$g){if(!information_schema($g))$rc[]=$g;}echo"<span id='label-db'>",lang(231),":</span> ",html_select("db",$rc,$K["db"]!=""?$K["db"]:$_GET["db"],$Ch,"label-db");}echo
input_hidden("change-js"),"<noscript><input type='submit' class='button' name='change' value='",lang(232),"'></noscript>","</p>\n","<table>","<thead><tr><th id='label-source'>",lang(168),"<th id='label-target'>",lang(169),"</thead>\n";$Cf=0;foreach($K["source"]as$t=>$X){echo"<tr>","<td>".html_select("source[".(+$t)."]",[-1=>""]+$uk,$X,($Cf==count($K["source"])-1?"foreignAddRow.call(this);":""),"label-source"),"<td>".html_select("target[".(+$t)."]",$ll,isset($K["target"][$t])?$K["target"][$t]:null,"","label-target");$Cf++;}echo"</table>\n","<noscript><p><input type='submit' class='button' name='add' value='",lang(233),"'></p></noscript>","<p>\n","<span id='label-delete'>".lang(93),":</span> ",html_select("on_delete",[-1=>""]+Driver::get()->getOnActions(),$K["on_delete"],"","label-delete"),"<span id='label-update'>".lang(92),":</span> ",html_select("on_update",[-1=>""]+Driver::get()->getOnActions(),$K["on_update"],"","label-update");if(DRIVER=='pgsql')echo
html_select("deferrable",['NOT DEFERRABLE','DEFERRABLE','DEFERRABLE INITIALLY DEFERRED'],$K["deferrable"]);echo
doc_link(['sql'=>"innodb-foreign-key-constraints.html",'mariadb'=>"architecture/server-constraints/foreign-key-constraints",]),"</p>\n<p>","<input type='submit' class='button default' value='",lang(113),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["view"])){$a=$_GET["view"];$K=$_POST;$Yh="VIEW";if(DIALECT=="pgsql"&&$a!=""){$P=table_status1($a);$Yh=strtoupper($P["Engine"]);}if($_POST){$A=trim($K["name"]);$La=" AS\n$K[select]";$x=ME."table=".urlencode($A);$_=lang(234);$U=($_POST["materialized"]?"MATERIALIZED VIEW":"VIEW");if(!$_POST["drop"]&&$a==$A&&DIALECT!="sqlite"&&$U=="VIEW"&&$Yh=="VIEW")query_redirect((DIALECT=="mssql"?"ALTER":"CREATE OR REPLACE")." VIEW ".table($A).$La,$x,$_);else{$nl=$A."_adminneo_".uniqid();drop_create("DROP $Yh ".table($a),"CREATE $U ".table($A).$La,"DROP $U ".table($A),"CREATE $U ".table($nl).$La,"DROP $U ".table($nl),($_POST["drop"]?substr(ME,0,-1):$x),lang(235),$_,lang(236),$a,$A);}}if(!$_POST&&$a!=""){$K=view($a);$K["name"]=$a;$K["materialized"]=($Yh!="VIEW");if($i=error())Admin::get()->addError($i);}if($a!="")page_header(lang(36).": ".h($a),["table"=>$a,lang(36)]);else
page_header(lang(237),[lang(237)]);echo"<form action='' method='post'>\n","<p>",lang(217),":","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>\n";if(support("materializedview"))echo
checkbox("materialized",1,$K["materialized"],lang(161));echo"</p>\n<p>";textarea("select",$K["select"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(113),"'>\n";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>\n",confirm(lang(211,$a));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["event"])){$ea=$_GET["event"];$qf=["YEAR","QUARTER","MONTH","DAY","HOUR","MINUTE","WEEK","SECOND","YEAR_MONTH","DAY_HOUR","DAY_MINUTE","DAY_SECOND","HOUR_MINUTE","HOUR_SECOND","MINUTE_SECOND"];$Ck=["ENABLED"=>"ENABLE","DISABLED"=>"DISABLE","SLAVESIDE_DISABLED"=>"DISABLE ON SLAVE"];$K=$_POST;if($_POST){if($_POST["drop"])query_redirect("DROP EVENT ".idf_escape($ea),substr(ME,0,-1),lang(238));elseif(in_array($K["INTERVAL_FIELD"],$qf)&&isset($Ck[$K["STATUS"]])){$Kj="\nON SCHEDULE ".($K["INTERVAL_VALUE"]?"EVERY ".q($K["INTERVAL_VALUE"])." $K[INTERVAL_FIELD]".($K["STARTS"]?" STARTS ".q($K["STARTS"]):"").($K["ENDS"]?" ENDS ".q($K["ENDS"]):""):"AT ".q($K["STARTS"]))." ON COMPLETION".($K["ON_COMPLETION"]?"":" NOT")." PRESERVE";queries_redirect(substr(ME,0,-1),($ea!=""?lang(239):lang(240)),(bool)queries(($ea!=""?"ALTER EVENT ".idf_escape($ea).$Kj.($ea!=$K["EVENT_NAME"]?"\nRENAME TO ".idf_escape($K["EVENT_NAME"]):""):"CREATE EVENT ".idf_escape($K["EVENT_NAME"]).$Kj)."\n".$Ck[$K["STATUS"]]." COMMENT ".q($K["EVENT_COMMENT"]).rtrim(" DO\n$K[EVENT_DEFINITION]",";").";"));}}if($ea!="")page_header(lang(241).": ".h($ea),[lang(241)]);else
page_header(lang(242),[lang(242)]);if(!$K&&$ea!=""){$L=get_rows("SELECT * FROM information_schema.EVENTS WHERE EVENT_SCHEMA = ".q(DB)." AND EVENT_NAME = ".q($ea));$K=reset($L);}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(217),"</th><td>","<input class='input' name='EVENT_NAME' value='",h($K["EVENT_NAME"]),"' data-maxlength='64' autocapitalize='off'>","</td></tr>\n","<tr><th title='datetime'>",lang(243),"</th><td>","<input class='input' name='STARTS' value='",h("$K[EXECUTE_AT]$K[STARTS]"),"'>","</td></tr>\n","<tr><th title='datetime'>",lang(244),"</th><td>","<input class='input' name='ENDS' value='",h($K["ENDS"]),"'>","</td></tr>\n","<tr><th>",lang(245),"</th><td>","<input type='number' name='INTERVAL_VALUE' value='",h($K["INTERVAL_VALUE"]),"' class='input size'> ",html_select("INTERVAL_FIELD",$qf,$K["INTERVAL_FIELD"]),"</td></tr>\n","<tr><th>",lang(152),"</th><td>",html_select("STATUS",$Ck,$K["STATUS"]),"</td></tr>\n","<tr><th>",lang(46),"</th><td>","<input class='input' name='EVENT_COMMENT' value='",h($K["EVENT_COMMENT"]),"' data-maxlength='64'>","</td></tr>\n","<tr><th></th><td>",checkbox("ON_COMPLETION","PRESERVE",$K["ON_COMPLETION"]=="PRESERVE",lang(246)),"</td></tr>\n","</table>\n","<p>";textarea("EVENT_DEFINITION",$K["EVENT_DEFINITION"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(113),"'>";if($ea!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$ea));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["procedure"])){$oa=($_GET["name"]?:$_GET["procedure"]);$_j=(isset($_GET["function"])?"FUNCTION":"PROCEDURE");$K=$_POST;$K["fields"]=(array)$K["fields"];if($_POST&&!process_fields($K["fields"])){foreach($K["fields"]as$t=>$j){if($j["field"]=="")unset($K["fields"][$t]);}$yh=routine_id($oa,routine($_GET["procedure"],$_j));$hh=routine_id($K["name"],$K);$cc=create_routine($_j,$K);$x=substr(ME,0,-1);$_=lang(247);if(!$_POST["drop"]&&$yh==$hh&&(DIALECT!="sql"||Connection::get()->isMariaDB()))query_redirect(substr_replace($cc,' OR REPLACE',6,0),$x,$_);else{$nl="$K[name]_adminer_".uniqid();drop_create("DROP $_j $yh",$cc,"DROP $_j $hh",create_routine($_j,["name"=>$nl]+$K),"DROP $_j ".routine_id($nl,$K),$x,lang(248),$_,lang(249),$oa,$K["name"]);}}if($oa!=""){$T=isset($_GET["function"])?lang(250):lang(251);page_header($T.": ".h($oa),[$T]);}else{$T=isset($_GET["function"])?lang(252):lang(253);page_header($T,[$T]);}if(!$_POST){if($oa=="")$K["language"]="sql";else{$K=routine($_GET["procedure"],$_j);$K["name"]=$oa;}}$pb=get_vals("SHOW CHARACTER SET");sort($pb);$Aj=routine_languages();echo"<form action='' method='post' id='form'>\n","<p>",lang(217),": ","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>";if($Aj)echo"<span id='label-language'>",lang(9),":</span> ",html_select("language",$Aj,$K["language"],"","label-language");echo"<input type='submit' class='button default' value='",lang(113),"'>","</p>\n","<div class='scrollable'>\n","<table class='nowrap' id='edit-fields'>\n";edit_fields($K["fields"],$pb,$_j);if(isset($_GET["function"])){echo"<tbody><tr>";if(support("move_col"))echo"<th></th>";echo"<th>",lang(254),"</th>";edit_type("returns",(array)$K["returns"],$pb,[],(DIALECT=="pgsql"?["void","trigger"]:[]));echo"<td></td>","</tr></tbody>\n";}echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>";textarea("definition",$K["definition"],20);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(113),"'>";if($oa!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$oa));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["check"])){$a=$_GET["check"];$A=$_GET["name"];$K=$_POST;if($K){if(DIALECT=="sqlite")$Jk=recreate_table($a,$a,[],[],[],"",[],"$A",($K["drop"]?"":$K["clause"]));else{$Jk=($A==""||queries("ALTER TABLE ".table($a)." DROP CONSTRAINT ".idf_escape($A)));if(!$K["drop"])$Jk=(bool)queries("ALTER TABLE ".table($a)." ADD".($K["name"]!=""?" CONSTRAINT ".idf_escape($K["name"]):"")." CHECK ($K[clause])");}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(255):($A!=""?lang(256):lang(257))),$Jk);}page_header(($A!=""?lang(258).": ".h($A):lang(173)),["table"=>$a]);if(!$K){$ub=Driver::get()->checkConstraints($a);$K=["name"=>$A,"clause"=>$ub[$A]];}echo"<form action='' method='post'>\n","<p>";if(DIALECT!="sqlite")echo
lang(217).': <input name="name" value="'.h($K["name"]).'" class="input" data-maxlength="64" autocapitalize="off"> ';echo
doc_link(['sql'=>"create-table-check-constraints.html",'mariadb'=>"reference/sql-statements/data-definition/constraint",],"?"),"</p>\n<p>";textarea("clause",$K["clause"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(113),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["trigger"])){$a=$_GET["trigger"];$A=isset($_GET["name"])?$_GET["name"]:"";$Jl=trigger_options();$K=trigger($A,$a)+["Trigger"=>$a."_bi"];if($_POST){if(in_array($_POST["Timing"],$Jl["Timing"])&&in_array($_POST["Event"],$Jl["Event"])&&in_array($_POST["Type"],$Jl["Type"])){$Ah=" ON ".table($a);$Tc="DROP TRIGGER ".idf_escape($A).(DIALECT=="pgsql"?$Ah:"");$x=ME."table=".urlencode($a);if($_POST["drop"])query_redirect($Tc,$x,lang(259));else{if($A!="")queries($Tc);queries_redirect($x,($A!=""?lang(260):lang(261)),(bool)queries(create_trigger($Ah,$_POST)));if($A!="")queries(create_trigger($Ah,$K+["Type"=>reset($Jl["Type"])]));}}$K=$_POST;}if($A!="")page_header(lang(262).": ".h($A),["table"=>$a,h($A)]);else
page_header(lang(263),["table"=>$a,lang(263)]);echo"<form action='' method='post' id='form'>\n","<table class='box box-light'>\n","<tr><th id='label-time'>",lang(264),"</th><td>",html_select("Timing",$Jl["Timing"],$K["Timing"],"triggerChange(/^".js_escape_re($a)."_[ba][iud]$/, '".js_escape($a)."', this.form);","label-time"),"</td></tr>\n","<tr><th id='label-event'>",lang(265),"</th><td>",html_select("Event",$Jl["Event"],$K["Event"],"this.form['Timing'].onchange();","label-event");if(in_array("UPDATE OF",$Jl["Event"]))echo" <input name='Of' value='".h($K["Of"])."' class='input hidden'>";echo"</td></tr>\n","<tr><th id='label-type'>",lang(44),"</th><td>",html_select("Type",$Jl["Type"],$K["Type"],"","label-type"),"</td></tr>\n","</table>\n","<p>",lang(217),"<input class='input' name='Trigger' value='",h($K["Trigger"]),"' data-maxlength='64' autocapitalize='off'>","</p>\n",script("gid('form')['Timing'].onchange();"),"<p>";textarea("Statement",$K["Statement"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(113),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>",confirm(lang(211,$A));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["user"])){$qa=$_GET["user"];$Ri=[""=>["All privileges"=>""]];foreach(get_rows("SHOW PRIVILEGES")as$K){foreach(explode(",",($K["Privilege"]=="Grant option"?"":$K["Context"]))as$Yb)$Ri[$Yb=="File access on server"?"Server Admin":$Yb][$K["Privilege"]]=$K["Comment"];}unset($Ri["Server Admin"]["Usage"]);foreach($Ri["Tables"]as$t=>$X)unset($Ri["Databases"][$t]);$gh=[];if($_POST){foreach($_POST["objects"]as$t=>$X)$gh[$X]=(array)$gh[$X]+(array)$_POST["grants"][$t];}$we=[];if(isset($_GET["host"])&&($I=Connection::get()->query("SHOW GRANTS FOR ".q($qa)."@".q($_GET["host"])))){while($K=$I->fetchRow()){if(preg_match('~GRANT (.*) ON (.*) TO ~',$K[0],$y)&&preg_match_all('~ *([^(,]*[^ ,(])( *\([^)]+\))?~',$y[1],$z,PREG_SET_ORDER)){foreach($z
as$X){if($X[1]!="USAGE")$we["$y[2]$X[2]"][$X[1]]=true;if(preg_match('~ WITH GRANT OPTION~',$K[0]))$we["$y[2]$X[2]"]["GRANT OPTION"]=true;}}}}$zi=!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8");if($_POST){$_h=(isset($_GET["host"])?q($qa)."@".q($_GET["host"]):"''");if($_POST["drop"])query_redirect("DROP USER $_h",ME."privileges=",lang(266));else{$jh=q($_POST["user"])."@".q($_POST["host"]);$si=$_POST["pass"];$fc=false;$I=true;if($_h!=$jh){$fc=(bool)queries("CREATE USER $jh IDENTIFIED BY ".($_POST["hashed"]?"PASSWORD ":"").q($si));$I=$fc;}elseif($si!="")$I=(bool)queries("SET PASSWORD FOR $jh = ".($zi||$_POST["hashed"]?q($si):"PASSWORD(".q($si).")"));if($I){$xj=[];foreach($gh
as$rh=>$ue){if(isset($_GET["grant"]))$ue=array_filter($ue);$ue=array_keys($ue);if(isset($_GET["grant"]))$xj=array_diff(array_keys(array_filter($gh[$rh],'strlen')),$ue);elseif($_h==$jh){$xh=array_keys((array)$we[$rh]);$xj=array_diff($xh,$ue);$ue=array_diff($ue,$xh);unset($we[$rh]);}if(preg_match('~^(.+)\s*(\(.*\))?$~U',$rh,$y)&&(!grant(false,$xj,$y[2],$y[1],$jh)||!grant(true,$ue,$y[2],$y[1],$jh))){$I=false;break;}}}if($I&&isset($_GET["host"])){if($_h!=$jh)queries("DROP USER $_h");elseif(!isset($_GET["grant"])){foreach($we
as$rh=>$xj){if(preg_match('~^(.+)(\(.*\))?$~U',$rh,$y))grant(false,array_keys($xj),$y[2],$y[1],$jh);}}}queries_redirect(ME."privileges=",(isset($_GET["host"])?lang(267):lang(268)),$I);if($fc)Connection::get()->query("DROP USER $jh");}}$T=isset($_GET["host"])?lang(28).": ".h("$qa@$_GET[host]"):lang(183);$yl=isset($_GET["host"])?h($qa):lang(183);page_header($T,["privileges"=>['',lang(72)],$yl]);if($_POST){$K=$_POST;$we=$gh;}else{$K=$_GET+["host"=>Connection::get()->getValue("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', -1)")];if($we)$we[".*"]=[];elseif(DB!="")$we[idf_escape(addcslashes(DB,"%_\\")).".*"]=[];else$we["*.* "]=[];}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(5),"</th>","<td><input class='input' name='host' data-maxlength='60' value='",h($K["host"]),"' autocapitalize='off'></td>\n","<tr><th>",lang(28),"</th>","<td><input class='input' name='user' data-maxlength='80' value='",h($K["user"]),"' autocapitalize='off'></td>\n",'<tr><th>',lang(29),"</th>","<td><input class='input' name='pass' id='pass' value='",h($K["pass"]),"' autocomplete='new-password'>";if(!$zi)echo
checkbox("hashed",1,$K["hashed"],lang(269),"typePassword(this.form['pass'], this.checked);");echo"</td>\n";if(!$K["hashed"])echo
script("typePassword(gid('pass'));");echo"</table>\n","<div class='scrollable'><table class='checkable'>\n","<thead><tr><th colspan='2'>".lang(72).doc_link(['sql'=>"grant.html#priv_level","mariadb"=>"reference/sql-statements/account-management-sql-statements/grant#privilege-levels"])."</th>";$p=0;foreach($we
as$rh=>$ue){echo"<th>";if($rh=="*.*")echo"*.*",input_hidden("objects[$p]","*.*");else
echo"<input class='input' name='objects[$p]' value='".h(trim($rh))."' size='10' autocapitalize='off'>";echo"</th>";$p++;}echo"</tr></thead>\n";foreach([""=>"","Server Admin"=>lang(5),"Databases"=>lang(30),"Tables"=>lang(8),"Procedures"=>lang(270),]as$Yb=>$Ac){foreach((array)$Ri[$Yb]as$Qi=>$Kb){echo"<tr>";if($Ac)echo"<td>$Ac</td>";echo"<td".(!$Ac?" colspan='2'":"").' lang="en" title="'.h($Kb).'">'.h($Qi)."</td>";$p=0;foreach($we
as$rh=>$ue){$A="'grants[$p][".h(strtoupper($Qi))."]'";$Y=$ue[strtoupper($Qi)];$Vi=strpos($rh,"@")!==false;$fh=$rh==".*";$Ca=$Qi=="All privileges";$ve=$Qi=="Grant option";if($rh=="*.*"&&$Qi=="Proxy")echo"<td></td>";elseif($Vi&&$Qi!="Proxy"&&!$ve)echo"<td></td>";elseif($Yb=="Server Admin"&&$rh!=(isset($we["*.*"])?"*.*":".*")&&!(($Vi||$fh)&&$Qi=="Proxy"))echo"<td></td>";elseif(isset($_GET["grant"]))echo"<td><select name=$A>"."<option></option>"."<option value='1'".($Y?" selected":"").">".lang(271)."</option>"."<option value='0'".($Y=="0"?" selected":"").">".lang(272)."</option>"."</select></td>";else{echo"<td class='center'><label class='block'>","<input type='checkbox' name=$A value='1'".($Y?" checked":"").($Ca?" id='grants-$p-all'":(!$ve?" class='grants-$p'":"")).">";if($Ca)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheckAll('.grants-$p'); };");elseif(!$ve)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheck('grants-$p-all'); };");echo"</label>";}$p++;}echo"</tr>";}}echo"</table></div>\n","<p>","<input type='submit' class='button default' value='",lang(113),"'>\n";if(isset($_GET["host"]))echo"<input type='submit' class='button' name='drop' value='",lang(160),"'>\n",confirm(lang(211,"$qa@$_GET[host]"));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["processlist"])){if(support("kill")){if($_POST){$Mf=0;foreach((array)$_POST["kill"]as$X){if(kill_process($X))$Mf++;}queries_redirect(ME."processlist=",lang(273,$Mf),$Mf||!$_POST["kill"]);}}page_header(lang(150),[lang(150)]);echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap checkable'>\n";$p=-1;foreach(process_list()as$p=>$K){if(!$p){echo"<thead><tr lang='en'>".(support("kill")?"<th>":"");foreach($K
as$t=>$X)echo"<th>$t".doc_link(['sql'=>"show-processlist.html#processlist_".strtolower($t),'mariadb'=>"reference/sql-statements/administrative-sql-statements/show/show-processlist",]);echo"</thead>\n","<tbody>\n";}echo"<tr>".(support("kill")?"<td>".checkbox("kill[]",$K[DIALECT=="sql"?"Id":"pid"],0):"");foreach($K
as$t=>$X)echo"<td>".($X!=""&&((DIALECT=="sql"&&$t=="Info"&&preg_match("~Query|Killed~",$K["Command"]))||(DIALECT=="pgsql"&&$t=="query")||(DIALECT=="oracle"&&$t=="sql_text"))?"<code class='jush-".DIALECT."'>".truncate_utf8($X,100).'</code> <a href="'.h(ME.($K["db"]!=""?"db=".urlencode($K["db"])."&":"")."sql=".urlencode($X)).'">'.icon("edit").lang(274).'</a>':h($X));echo"\n";}if($p>=0)echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: partialArg(tableClick, true)});");echo"</table>\n","</div>\n","<p>";if(support("kill"))echo($p+1)."/".lang(275,max_connections()),"<p><input type='submit' class='button' value='".lang(276)."'>\n";echo
input_token(),"</p>\n","</form>\n",script("tableCheck();");}elseif(isset($_GET["select"])){$a=$_GET["select"];$R=table_status1($a);$s=indexes($a);$k=fields($a);$ee=column_foreign_keys($a);$th=$R["Oid"];$yj=[];$c=[];$Qj=[];$Oh=[];$rl=null;foreach($k
as$t=>$j){$A=Admin::get()->getFieldName($j);$bh=html_entity_decode(strip_tags($A),ENT_QUOTES);if(isset($j["privileges"]["select"])&&$A!=""){$c[$t]=$bh;if(is_shortable($j))$rl=Admin::get()->processSelectionLength();}if(isset($j["privileges"]["where"])&&$A!="")$Qj[$t]=$bh;if(isset($j["privileges"]["order"])&&$A!="")$Oh[$t]=$bh;$yj+=$j["privileges"];}list($M,$xe)=Admin::get()->processSelectionColumns($c,$s);$M=array_unique($M);$xe=array_unique($xe);$wf=count($xe)<count($M);$Z=Admin::get()->processSelectionSearch($k,$s);$D=Admin::get()->processSelectionOrder($k,$s);$v=Admin::get()->processSelectionLimit();if($_GET["modify"]&&!Admin::get()->isDataEditAllowed())redirect(ME."select=".urlencode($a));if($_GET["val"]&&is_ajax()){header("Content-Type: text/plain; charset=utf-8");foreach($_GET["val"]as$Ul=>$K){$La=convert_field($k[key($K)]);$M=[$La?:idf_escape(key($K))];$Z[]=where_check($Ul,$k);$J=Driver::get()->select($a,$M,$Z,$M);if($J)echo
first($J->fetchRow());}exit;}$Ni=$Xl=[];foreach($s
as$r){if($r["type"]=="PRIMARY"){$Ni=array_flip($r["columns"]);$Xl=($M?$Ni:[]);foreach($Xl
as$t=>$X){if(in_array(idf_escape($t),$M))unset($Xl[$t]);}break;}}if($th&&!$Ni){$Ni=$Xl=[$th=>0];$s[]=["type"=>"PRIMARY","columns"=>[$th]];}$O=Admin::get()->getSettings();if($_POST){$Em=$Z;if(!$_POST["all"]&&is_array($_POST["check"])){$ub=[];foreach($_POST["check"]as$qb)$ub[]=where_check($qb,$k);$Em[]="((".implode(") OR (",$ub)."))";}$Em=($Em?"\nWHERE ".implode(" AND ",$Em):"");if($_POST["export"]){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers($a);Admin::get()->dumpTable($a,"");$me=($M?implode(", ",$M):"*").convert_fields($c,$k,$M)."\nFROM ".table($a);$_e=($xe&&$wf?"\nGROUP BY ".implode(", ",$xe):"").($D?"\nORDER BY ".implode(", ",$D):"");if(!is_array($_POST["check"])||$Ni)$H="SELECT $me$Em$_e";else{$Rl=[];foreach($_POST["check"]as$X)$Rl[]="(SELECT".limit($me,"\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$k).$_e,1).")";$H=implode(" UNION ALL ",$Rl);}Admin::get()->dumpData($a,"table",$H);exit;}if($_POST["save"]||$_POST["delete"]){$I=true;$za=0;$kk=[];if(!$_POST["delete"]){$Yj=array_keys($_POST["fields"]+$_POST["function"]);foreach($Yj
as$A){$X=process_input($k[$A]);if($X!==null&&($_POST["clone"]||$X!==false))$kk[idf_escape($A)]=($X!==false?$X:idf_escape($A));}}if($_POST["delete"]||$kk){if($_POST["clone"])$H="INTO ".table($a)." (".implode(", ",array_keys($kk)).")\nSELECT ".implode(", ",$kk)."\nFROM ".table($a);if($_POST["all"]||($Ni&&is_array($_POST["check"]))||$wf){$I=($_POST["delete"]?Driver::get()->delete($a,$Em):($_POST["clone"]?queries("INSERT $H$Em".Driver::get()->getInsertReturningSql($a)):Driver::get()->update($a,$kk,$Em)));$za=Connection::get()->getAffectedRows();if(is_object($I))$za+=$I->getRowsCount();}else{foreach((array)$_POST["check"]as$X){$Dm="\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$k);$I=($_POST["delete"]?Driver::get()->delete($a,$Dm,1):($_POST["clone"]?queries("INSERT".limit1($a,$H,$Dm)):Driver::get()->update($a,$kk,$Dm,1)));if(!$I)break;$za+=Connection::get()->getAffectedRows();}}}$_=lang(277,$za);if($_POST["clone"]&&$I&&$za==1){$Vf=last_id($I);if($Vf)$_=lang(205," $Vf");}queries_redirect(remove_from_uri($_POST["all"]&&$_POST["delete"]?"page":""),$_,(bool)$I);if(!$_POST["delete"]){$ed=array_filter($k,function($j){return!(isset($j["generated"])?$j["generated"]:null);});edit_form($a,$ed,(array)$_POST["fields"],!$_POST["clone"]);page_footer();exit;}}elseif(!$_POST["import"]){if(!$_POST["val"])Admin::get()->addError(lang(278));else{$Jk=true;$za=0;foreach($_POST["val"]as$Ul=>$K){$kk=[];foreach($K
as$t=>$X){$t=bracket_escape($t,true);$kk[idf_escape($t)]=(preg_match('~char|text~',$k[$t]["type"])||$X!=""?Admin::get()->processFieldInput($k[$t],$X):"NULL");}$Jk=(bool)Driver::get()->update($a,$kk," WHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($Ul,$k),($wf||$Ni?0:1)," ");if(!$Jk)break;$za+=Connection::get()->getAffectedRows();}queries_redirect(remove_from_uri(),lang(277,$za),$Jk);}}elseif(!is_string($l=get_file("csv_file",true)))Admin::get()->addError(upload_error($l));elseif(!preg_match('~~u',$l))Admin::get()->addError(lang(279));else{$O->updateParameter("exportFormat",$_POST["import_format"]);$Fb=array_keys($k);preg_match_all('~(?>"[^"]*"|[^"\r\n]+)+~',$l,$z);$za=count($z[0]);Driver::get()->begin();$Zj=($_POST["import_format"]=="csv;"?";":($_POST["import_format"]=="tsv"?"\t":","));$L=[];foreach($z[0]as$t=>$X){preg_match_all("~((?>\"[^\"]*\")+|[^$Zj]*)$Zj~",$X.$Zj,$ug);if(!$t&&!array_diff($ug[1],$Fb)){$Fb=$ug[1];$za--;}else{$kk=[];foreach($ug[1]as$p=>$_b)$kk[idf_escape($Fb[$p])]=($_b==""&&$k[$Fb[$p]]["null"]?"NULL":q(preg_match('~^".*"$~s',$_b)?str_replace('""','"',substr($_b,1,-1)):$_b));$L[]=$kk;}}$Jk=!$L||Driver::get()->insertUpdate($a,$L,$Ni);if($Jk)Driver::get()->commit();queries_redirect(remove_from_uri("page"),lang(280,$za),$Jk);Driver::get()->rollback();}}$Zk=Admin::get()->getTableName($R);if(is_ajax()){page_headers();ob_start();}else
page_header(lang(55).": $Zk",[$Zk]);$nf=null;if(isset($yj["insert"])||!support("table")){$nf=[];foreach((array)$_GET["where"]as$X){if(isset($ee[$X["col"]])&&count($ee[$X["col"]])==1&&($X["op"]=="="||(!$X["op"]&&(is_array($X["val"])||!preg_match('~[_%]~',$X["val"])))))$nf["preset"."[".bracket_escape($X["col"])."]"]=$X["val"];}}Admin::get()->printTableMenu($R,$nf);if(!$c&&support("table"))echo"<p class='error'>".lang(281).($k?".":": ".error())."\n";else{echo"<form id='form' action=''>\n","<div hidden>";hidden_fields_get();if(DB!=""){echo
input_hidden("db",DB);if(isset($_GET["ns"]))echo
input_hidden("ns",$_GET["ns"]);}echo
input_hidden("select",$a),"<input type='submit' class='button' value='".lang(55)."'>","</div>\n","<div class='field-sets'>\n";Admin::get()->printSelectionColumns($M,$c);Admin::get()->printSelectionSearch($Z,$Qj,$s);Admin::get()->printSelectionOrder($D,$Oh,$s);Admin::get()->printSelectionLimit($v);Admin::get()->printSelectionLength($rl);Admin::get()->printSelectionAction($s);echo"</div>\n</form>\n";$E=isset($_GET["page"])?$_GET["page"]:null;if($E=="last"){$ke=Connection::get()->getValue(count_rows($a,$Z,$wf,$xe));$E=(int)floor(max(0,intval($ke)-1)/$v);}else{$ke=false;$E=(int)$E;}$Rj=$M;$ye=$xe;if(!$Rj){$Rj[]="*";$Zb=convert_fields($c,$k,$M);if($Zb)$Rj[]=substr($Zb,2);}foreach($M
as$t=>$X){$j=$k[idf_unescape($X)];if($j&&($La=convert_field($j)))$Rj[$t]="$La AS $X";}if(DIALECT=="pgsql"||DIALECT=="mssql"){foreach((array)$_GET["columns"]as$t=>$X){if(isset($Rj[$t])&&$X["fun"])$Rj[$t].=" AS ".idf_escape(apply_sql_function($X["fun"],($X["col"]!=""?$X["col"]:"*")));}}if(!$wf&&$Xl){foreach($Xl
as$t=>$X){$Rj[]=idf_escape($t);if($ye)$ye[]=idf_escape($t);}}$I=Driver::get()->select($a,$Rj,$Z,$ye,$D,$v,$E,true);if(!$I)echo"<p class='error'>".error()."\n";else{if(DIALECT=="mssql"&&$E)$I->seek($v*$E);echo"<form id='selection_form' action='' method='post' enctype='multipart/form-data'>\n","<div class='table-footer-parent'>\n";$L=[];while($K=$I->fetchAssoc()){if($E&&DIALECT=="oracle")unset($K["RNUM"]);$L[]=$K;}if($_GET["page"]!="last"&&$v&&$xe&&$wf&&DIALECT=="sql")$ke=Connection::get()->getValue(" SELECT FOUND_ROWS()");$fd=false;if(!$L)echo"<p class='message'>".lang(89)."\n";else{$Va=Admin::get()->getBackwardKeys($a,$Zk);echo"<div class='scrollable'>\n","<table id='table' class='nowrap checkable'>\n","<thead><tr>";if($xe||!$M){echo"<th class='actions'><input type='checkbox' id='all-page' class='jsonly'>".script("gid('all-page').onclick = partial(formCheck, /check/);","");if(Admin::get()->isDataEditAllowed())echo" <a href='",h($_GET["modify"]?remove_from_uri("modify"):$_SERVER["REQUEST_URI"]."&modify=1")."' title='",lang(282),"'>",icon_solo("edit-all"),"</a>";}$ch=[];$pe=[];reset($M);$dj=1;foreach($L[0]as$t=>$X){if(!isset($Xl[$t])){$Tj=key($M);$X=isset($_GET["columns"][$Tj])?$_GET["columns"][$Tj]:[];$j=$k[$M?($X?$X["col"]:current($M)):$t];$A=($j?Admin::get()->getFieldName($j,$dj):(isset($X["fun"])?"*":h($t)));if($A!=""){$dj++;$ch[$t]=$A;$b=idf_escape($t);$Re=remove_from_uri('(order|desc)[^=]*|page').'&order%5B0%5D='.urlencode($t);$Ac="&desc%5B0%5D=1";echo"<th id='th[".h(bracket_escape($t))."]'>";$oe=apply_sql_function(isset($X["fun"])?$X["fun"]:null,$A);$tk=isset($j["privileges"]["order"])||(isset($X["fun"])?$X["fun"]:null);if($tk)echo'<a href="',h($Re.($D[0]==$b||$D[0]==$t?$Ac:'')),'">',"$oe</a>";else
echo$oe;echo"<span class='column'>";if($tk)echo"<a href='".h($Re.$Ac)."' title='".lang(62)."' class='button light'>",icon_solo("arrow-down"),"</a>";if(!isset($X["fun"])&&isset($j["privileges"]["where"]))echo"<a href='#fieldset-search' title='".lang(59)."' class='button light jsonly'>",icon_solo("search"),"</a>",script("qsl('a').onclick = partial(selectSearch, '".js_escape($t)."');");echo"</span>";}$pe[$t]=isset($X["fun"])?$X["fun"]:null;next($M);}}$dg=[];if($_GET["modify"]){foreach($L
as$K){foreach($K
as$t=>$X)$dg[$t]=max($dg[$t],min(40,strlen(utf8_decode($X))));}}if($Va)echo"<th>".lang(17)."</th>";echo"</thead>\n","<tbody>\n";if(is_ajax())ob_end_clean();foreach(Admin::get()->fillForeignDescriptions($L,$ee)as$Zg=>$K){$Tl=unique_array($L[$Zg],$s);if(!$Tl){$Tl=[];reset($M);foreach($L[$Zg]as$t=>$X){if(!preg_match('~^(COUNT|AVG|GROUP_CONCAT|MAX|MIN|SUM)\(~',current($M)))$Tl[$t]=$X;next($M);}}$Ul="";foreach($Tl
as$t=>$X){$j=isset($k[$t])?$k[$t]:null;if((DIALECT=="sql"||DIALECT=="pgsql")&&$j&&preg_match('~char|text|enum|set~',$j["type"])&&strlen($X)>64){$t=(strpos($t,'(')?$t:idf_escape($t));$t="MD5(".(DIALECT!='sql'||preg_match("~^utf8~",isset($j["collation"])?$j["collation"]:"")?$t:"CONVERT($t USING ".charset(Connection::get()).")").")";$X=md5($X);}$Ul
.="&".($X!==null?urlencode("where[".bracket_escape($t)."]")."=".urlencode($X===false?"f":$X):"null%5B%5D=".urlencode($t));}echo"<tr>";if($xe||!$M){echo"<td class='actions'>",checkbox("check[]",substr($Ul,1),in_array(substr($Ul,1),(array)$_POST["check"]));if(!$wf&&Admin::get()->isDataEditAllowed())echo" <a href='",h(ME."edit=".urlencode($a).$Ul),"' class='edit' title='",lang(38),"'>",icon_solo("edit"),"</a>";}reset($M);foreach($K
as$t=>$X){if(isset($ch[$t])){$b=current($M);$j=isset($k[$t])?$k[$t]:null;$w="";if($j&&is_blob($j)&&$X!="")$w=ME.'download='.urlencode($a).'&field='.urlencode($t).$Ul;if(!$w&&$X!==null){foreach((array)$ee[$t]as$n){if(count($ee[$t])==1||end($n["source"])==$t){$w="";foreach($n["source"]as$p=>$uk)$w
.=where_link($p,$n["target"][$p],$L[$Zg][$uk]);$w=($n["db"]!=""?preg_replace('~([?&]db=)[^&]+~','\1'.urlencode($n["db"]),ME):ME).'select='.urlencode($n["table"]).$w;if($n["ns"])$w=preg_replace('~([?&]ns=)[^&]+~','\1'.urlencode($n["ns"]),$w);if(count($n["source"])==1)break;}}}if($b=="COUNT(*)"){$w=ME."select=".urlencode($a);$p=0;foreach((array)$_GET["where"]as$W){if(!array_key_exists($W["col"],$Tl))$w
.=where_link($p++,$W["col"],$W["val"],$W["op"]);}foreach($Tl
as$Ef=>$W)$w
.=where_link($p++,$Ef,$W);}$oh=$X===null;$Se=select_value($X,$w,$j,$rl);$sd=bracket_escape($t);$q=h("val[$Ul][$sd]");$Ii=isset($_POST["val"][$Ul][$sd])?$_POST["val"][$Ul][$sd]:null;$Zl=isset($j["privileges"]["update"])?$j["privileges"]["update"]:false;$dd=!is_array($K[$t])&&is_utf8($Se)&&$L[$Zg][$t]==$K[$t]&&!$pe[$t]&&!(isset($j["generated"])?$j["generated"]:false);$U=($b&&preg_match('~^(AVG|MIN|MAX)\((.+)\)~',$b,$z)?$k[idf_unescape($z[2])]["type"]:(isset($j["type"])?$j["type"]:null));$Tg=$U=="money"||($b&&preg_match('~^SUM\((.+)\)~',$b,$z)&&$k[idf_unescape($z[1])]["type"])=="money";$pl=$U&&preg_match('~text|json|lob~',$U);$qh=($U&&preg_match(number_type(),$U))||($b&&preg_match('~^(CHAR_LENGTH|ROUND|FLOOR|CEIL|UNIX_TIMESTAMP|TIME_TO_SEC|COUNT|SUM)\(~',$b));$yb=$qh&&($oh||is_numeric(strip_tags($Se))||$Tg)?"class='number'":"";echo"<td id='$q' $yb";if(($_GET["modify"]&&$dd&&!$oh)||$Ii!==null){$fd=true;$Ce=h($Ii!==null?$Ii:$K[$t]);echo" data-editing='true'>".($pl?"<textarea name='$q' cols='30' rows='".(substr_count($K[$t],"\n")+1)."'>$Ce</textarea>":"<input class='input' name='$q' value='$Ce' size='$dg[$t]'>");}else{$rg=strpos($Se,"<i>…</i>");if($Zl)echo" data-text='".($rg?2:($pl?1:0))."'".($dd?"":" data-warning='".lang(283)."'");echo">$Se";}}next($M);}if($Va){echo"<td>";Admin::get()->printBackwardKeys($Va,$L[$Zg]);echo"</td>";}echo"</tr>\n";}if(is_ajax())exit;echo"</tbody>\n",script("mixin(qs('#table tbody'), {onclick: partialArg(tableClick, false, ".(Admin::get()->isDataEditAllowed()?"true":"false")."), ondblclick: partialArg(tableClick, true), onkeydown: onEditingKeydown});"),"</table>\n",script("initToggles(gid('table'));"),"</div>\n";}if(!is_ajax()){if($L||$E){$ud=true;if($_GET["page"]!="last"){if(!$v||(count($L)<$v&&($L||!$E)))$ke=($E?$E*$v:0)+count($L);elseif(DIALECT!="sql"||!$wf){$ke=($wf?false:found_rows($R,$Z));if($ke<max(1e4,2*($E+1)*$v))$ke=first(slow_query(count_rows($a,$Z,$wf,$xe)));elseif(DIALECT=='sql'||DIALECT=='pgsql')$ud=false;}}$ei=($v!==null&&($ke===false||$ke>$v||$E));if($ei){if(($ke===false?count($L)+1:$ke-$E*$v)>$v)echo'<p class="links">','<a href="',h(remove_from_uri("page")."&page=".($E+1)),'" class="loadmore">',icon("expand"),lang(284),'</a>',script("qsl('a').onclick = partial(loadNextPage, $v, '".js_escape(lang(285))."…');","");echo"\n";}echo"<div class='table-footer'><div class='field-sets'>\n";if($ei){$yg=($ke===false?$E+(count($L)>=$v?2:1):(int)floor(($ke-1)/$v));$Pc="<li>…</li>";echo"<fieldset>";if(DIALECT!="simpledb"){echo"<legend><a href='".h(remove_from_uri("page"))."'>".lang(286)."</a></legend>",script("qsl('a').onclick = function () { pageClick(this.href, +prompt('".js_escape(lang(286))."', '".($E+1)."')); return false; };"),"<div id='fieldset-pagination' class='fieldset-content'><ul class='pagination'>",pagination(0,$E);if($E>5)echo$Pc;for($p=max(1,$E-4);$p<min($yg,$E+5);$p++)echo
pagination($p,$E);if($yg>0){if($E+5<$yg)echo$Pc;echo($ud&&$ke!==false?pagination($yg,$E):" <a href='".h(remove_from_uri("page")."&page=last")."' title='~$yg'>".lang(287)."</a>");}echo"</ul></div>";}else{echo"<legend>".lang(286)."</legend>","<div id='fieldset-pagination'><ul class='pagination'>",pagination(0,$E);if($E>1)echo$Pc;if($E)echo
pagination($E,$E);if($yg>$E){echo
pagination($E+1,$E);if($yg>$E+1)echo$Pc;}echo"</ul></div>";}echo"</fieldset>\n";}echo"<fieldset>","<legend>".lang(288)."</legend><div class='fieldset-content'>";$Ic=($ud?"":"~ ").$ke;echo
checkbox("all",1,0,($ke!==false?($ud?"":"~ ").lang(187,$ke):""),"const checked = formChecked(this, /check/); selectCount('selected', this.checked ? '$Ic' : checked); selectCount('selected2', this.checked || !checked ? '$Ic' : checked);")."\n","</div></fieldset>\n";if(Admin::get()->isDataEditAllowed()){echo"<fieldset",($_GET["modify"]?'':' class="jsonly"'),">","<legend>",lang(282),"</legend>";$Fj=($_GET["modify"]?"":" data-inline-edit='1'".($fd?"":" disabled"));echo"<div class='fieldset-content'",($_GET["modify"]?"":" title='".lang(278)."'"),">","<input type='submit' class='button' id='modify-save' value='",lang(113),"'",$Fj,">","</div>","</fieldset>\n","<fieldset>","<legend>",lang(159)," <span id='selected'></span></legend>","<div class='fieldset-content'>","<input type='submit' class='button' name='edit' value='",lang(38),"'> ","<input type='submit' class='button' name='clone' value='",lang(274),"'> ","<input type='submit' class='button' name='delete' value='",lang(117),"'>",confirm(),"</div>","</fieldset>\n";}$ge=Admin::get()->getDumpFormats();foreach((array)$_GET["columns"]as$b){if($b["fun"]){unset($ge['sql']);break;}}if($ge){print_fieldset_start("export",lang(74)." <span id='selected2'></span>","export");echo
html_select("format",$ge,$O->getParameter("exportFormat"));$bi=Admin::get()->getDumpOutputs();echo($bi?" ".html_select("output",$bi,$O->getParameter("exportOutput")):"")," <input type='submit' class='button' name='export' value='".lang(74)."'>\n";print_fieldset_end("export");}echo"</div></div>\n",script("initTableFooter()");}echo"</div>\n";if(Admin::get()->isDataEditAllowed()){echo"<p>","<a href='#import'>",icon("import"),lang(73),"</a>",script("qsl('a').onclick = partial(toggle, 'import');",""),"</p>","<p id='import'",($_POST["import"]?"":" class='hidden'"),">";if(ini_bool("file_uploads"))echo"<input type='file' name='csv_file'> ",html_select("import_format",["csv"=>"CSV,","csv;"=>"CSV;","tsv"=>"TSV"],$O->getParameter("exportFormat"))," <input type='submit' class='button default' name='import' value='".lang(73)."'>",file_upload_form_script("selection_form","csv_file");else
echo
lang(194);echo"</p>";}echo
input_token(),"</form>\n",(!$xe&&$M?"":script("tableCheck();"));}else
echo"</div>\n";}}if(is_ajax()){ob_end_clean();exit;}}elseif(isset($_GET["variables"])){$P=isset($_GET["status"]);$T=$P?lang(152):lang(151);page_header($T,[$T]);$om=($P?Admin::get()->getStatusVariables():Admin::get()->getServerVariables());if(!$om)echo"<p class='message'>",lang(89),"</p>\n";else{echo"<div class='scrollable'><table>\n";foreach($om
as$K){echo"<tr>";$t=array_shift($K);echo"<th><code class='jush-".DIALECT.($P?"status":"set")."'>".h($t)."</code></th>";foreach($K
as$X)echo"<td>",nl2br(h($X)),"</td>";echo"</tr>\n";}echo"</table></div>\n";}}elseif(isset($_GET["script"])){header("Content-Type: text/javascript; charset=utf-8");if($_GET["script"]=="db"){$Mk=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$e=[];$pc=null;foreach(table_status()as$A=>$R){$e["Comment-$A"]=h($R["Comment"]);if(!is_view($R)||preg_match('~materialized~i',$R["Engine"])){$e["Engine-$A"]=h($R["Engine"]);$Bb=isset($R["Collation"])?$R["Collation"]:"";if($Bb==""){if($pc===null)$pc=db_collation(DB,collations())??"";$Bb=$pc;}$e["Collation-$A"]=h($Bb);foreach($Mk+["Auto_increment"=>0,"Rows"=>0]as$t=>$X){if($R[$t]!=""){$X=format_number($R[$t]);if($X>=0)$e["$t-$A"]=($t=="Rows"?format_rows($R):$X);if(isset($Mk[$t]))$Mk[$t]+=($R["Engine"]!="InnoDB"||$t!="Data_free"?$R[$t]:0);}elseif(array_key_exists($t,$R))$e["$t-$A"]="?";}}}if(function_exists('AdminNeo\db_status'))$Mk=db_status();foreach($Mk
as$t=>$X)$e["sum-$t"]=format_number($X);echo
json_encode($e,JSON_UNESCAPED_UNICODE);}elseif($_GET["script"]=="kill")Connection::get()->query("KILL ".number($_POST["kill"]));else{$e=[];foreach(count_tables(Admin::get()->getDatabases())as$g=>$X){$e["tables-$g"]=$X;$e["size-$g"]=db_size($g);}echo
json_encode($e,JSON_UNESCAPED_UNICODE);}exit;}else{$il=array_merge((array)$_POST["tables"],(array)$_POST["views"]);if($il&&!$_POST["search"]){$I=true;$_="";if(DIALECT=="sql"&&$_POST["tables"]&&count($_POST["tables"])>1&&($_POST["drop"]||$_POST["truncate"]||$_POST["copy"]))queries("SET foreign_key_checks = 0");if($_POST["truncate"]||$_POST["truncate_cascade"]){if($_POST["tables"])$I=truncate_tables($_POST["tables"],(bool)$_POST["truncate_cascade"]);$_=lang(289);}elseif($_POST["move"]){$I=move_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$_=lang(290);}elseif($_POST["copy"]){$I=copy_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$_=lang(291);}elseif($_POST["drop"]){if($_POST["views"])$I=drop_views($_POST["views"]);if($I&&$_POST["tables"])$I=drop_tables($_POST["tables"]);$_=lang(292);}elseif(DIALECT=="sqlite"&&$_POST["check"]){foreach((array)$_POST["tables"]as$Q){foreach(get_rows("PRAGMA integrity_check(".q($Q).")")as$K)$_
.="<b>".h($Q)."</b>: ".h($K["integrity_check"])."<br>";}}elseif(DIALECT!="sql"){$I=(DIALECT=="sqlite"?queries("VACUUM"):apply_queries("VACUUM".($_POST["optimize"]?" ANALYZE":""),$_POST["tables"]));$_=lang(293);}elseif(!$_POST["tables"])$_=lang(78);elseif($I=queries(($_POST["optimize"]?"OPTIMIZE":($_POST["check"]?"CHECK":($_POST["repair"]?"REPAIR":"ANALYZE")))." TABLE ".implode(", ",array_map('AdminNeo\idf_escape',$_POST["tables"])))){while($K=$I->fetchAssoc())$_
.="<b>".h($K["Table"])."</b>: ".h($K["Msg_text"])."<br>";}queries_redirect($_SERVER["REQUEST_URI"],$_,(bool)$I);}if($_GET["ns"]=="")page_header(lang(30).": ".h(DB),true);else
page_header(lang(182).": ".h($_GET["ns"]),true);Admin::get()->printDatabaseMenu();if($_GET["ns"]===""){echo"<h2 id='schemas'>".lang(294)."</h2>\n";$Mj=Admin::get()->getSchemas();if(!$Mj)echo"<p class='message'>".lang(295)."\n";else{echo"<div class='scrollable'>\n","<table class='nowrap'>\n",'<thead><tr class="wrap"><th>',lang(182),"</th></tr></thead>";foreach($Mj
as$A)echo"<tr><th><a href='",h(ME),"ns=".urlencode($A),"' title='",lang(296),"'>".h($A)."</a></th></tr>";echo'</table></div>';}echo'<p class="links"><a href="'.h(ME).'scheme=">'.icon("database-add").lang(76)."</a>\n";}else{echo"<h2 id='tables-views'>".lang(297)."</h2>\n";$dl=['sql'=>'show-table-status.html','mariadb'=>'reference/sql-statements/administrative-sql-statements/show/show-table-status'];$pc=db_collation(DB,collations());$c=["Engine"=>["label"=>lang(163),"doc"=>doc_link(['sql'=>'storage-engines.html','mariadb'=>'server-usage/storage-engines']),],];if($pc!="")$c["Collation"]=["label"=>lang(45),"doc"=>doc_link(['sql'=>'charset-charsets.html','mariadb'=>'reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations']),];$c+=["Data_length"=>["label"=>lang(298),"doc"=>doc_link($dl+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT','oracle'=>'REFRN20286']),"link"=>"create","title"=>lang(35),],"Index_length"=>["label"=>lang(299),"doc"=>doc_link($dl+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT']),"link"=>"indexes","title"=>lang(167),],"Data_free"=>["label"=>lang(300),"doc"=>doc_link($dl),"link"=>"edit","title"=>lang(7),],"Auto_increment"=>["label"=>lang(47),"doc"=>doc_link(['sql'=>'example-auto-increment.html','mariadb'=>'reference/data-types/auto_increment']),"link"=>"auto_increment=1&create","title"=>lang(35),],"Rows"=>["label"=>lang(301),"doc"=>doc_link($dl+['pgsql'=>'catalog-pg-class.html#CATALOG-PG-CLASS','oracle'=>'REFRN20286']),"link"=>"select","title"=>lang(33),],];if(support("comment"))$c["Comment"]=["label"=>lang(46),"doc"=>doc_link($dl+['pgsql'=>'functions-info.html#FUNCTIONS-INFO-COMMENT-TABLE']),];$D=(is_string($_GET["order"])?$_GET["order"]:"");$Bc=null;if(preg_match('~^(.+)-(asc|desc)$~',$D,$y)){$D=$y[1];$Bc=($y[2]=="desc");}if($D!="__table"&&!isset($c[$D]))$D="";if($Bc===null)$Bc=isset($c[$D]["link"]);$Gm=($D!=""&&$D!="__table")||support("fast_status");$gl=($Gm?table_status():tables_list());if(!$gl)echo"<p class='message'>".lang(78)."\n";else{echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n";if(support("table")){echo"<div class='field-sets'>\n","<fieldset><legend>".lang(302)." <span id='selected2'></span></legend><div class='fieldset-content'>",html_select("op",Admin::get()->getOperators(),isset($_POST["op"])?$_POST["op"]:Driver::get()->getLikeOperator()),"<input type='search' class='input' name='query' value='".h($_POST["query"])."'>",script("qsl('input').onkeydown = partialArg(bodyKeydown, 'search');","")," <input type='submit' class='button' name='search' value='".lang(59)."'>\n","</div></fieldset>\n","</div>\n";if($_POST["search"]&&$_POST["query"]!=""){$_GET["where"][0]["op"]=$_POST["op"];search_tables();}}echo"<div class='scrollable'>\n","<table class='nowrap checkable'>\n",'<thead><tr class="wrap">','<td class="actions"><input id="check-all" type="checkbox" class="input jsonly">'.script("gid('check-all').onclick = partial(formCheck, /^(tables|views)\[/);","");$ah=($D==""||$D=="__table");$Yk=($ah&&!$Bc?ME."order=__table-desc":substr(ME,0,-1));echo'<th><a href="'.h($Yk).'">'.lang(8).'</a>';foreach($c
as$t=>$b){$Dc=($t===$D?!$Bc:isset($b["link"]));echo'<td><a href="'.h(ME)."order=$t-".($Dc?"desc":"asc").'">'.$b["label"].'</a>'.$b["doc"];}echo"</thead>\n","<tbody>\n";if($D=="__table"){if($Bc)$gl=array_reverse($gl,true);}elseif($D){uasort($gl,function($sa,$Sa)use($D,$Bc){$Hm=isset($sa[$D])?$sa[$D]:null;$Im=isset($Sa[$D])?$Sa[$D]:null;$I=($Hm<$Im?-1:($Hm>$Im?1:0));return($Bc?-$I:$I);});}$Mk=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$S=0;foreach($gl
as$A=>$P){$sm=($Gm?is_view($P):$P!==null&&!preg_match('~table|sequence~i',$P));$ld=($Gm?(isset($P["Engine"])?$P["Engine"]:""):$P);$q=h("Table-".$A);echo'<tr><td class="actions">'.checkbox(($sm?"views[]":"tables[]"),$A,in_array("$A",$il,true),"","","",$q);if(!Admin::get()->getSettings()->isSelectionPreferred()&&(support("table")||support("indexes")))$ua="table";else$ua="select";echo"<th><a href='",h(ME),"$ua=",urlencode($A),"' id='$q'>",h($A),"</a></th>";if($sm&&!preg_match('~materialized~i',$ld)){$T=lang(162);$Gb=count($c)-(support("comment")?2:1);echo'<td colspan="'.$Gb.'">'.(support("view")?"<a href='".h(ME)."view=".urlencode($A)."' title='".lang(36)."'>$T</a>":$T),"<td align='right'><a href='".h(ME)."select=".urlencode($A)."' title='".lang(33)."'>?</a>";}else{foreach($c
as$t=>$b){if($t=="Comment")continue;$q=" id='$t-".h($A)."'";$w=isset($b["link"])?$b["link"]:"";if(!$w){$X="";if($Gm){$X=isset($P[$t])?$P[$t]:"";if($t=="Collation"&&$X=="")$X=$pc;}echo"<td$q>".h($X);continue;}$X="?";if($Gm){$B=isset($P[$t])?$P[$t]:"";if(is_numeric($B)&&$B>=0){$X=($t=="Rows"?format_rows($P):format_number($B));if(isset($Mk[$t])&&($ld!="InnoDB"||$t!="Data_free"))$Mk[$t]+=$B;}}echo"<td align='right'>".(support("table")||$t=="Rows"||(support("indexes")&&$t!="Data_length")?"<a href='".h(ME."$w=").urlencode($A)."'$q title='".$b["title"]."'>".h($X)."</a>":"<span$q>".h($X)."</span>");}$S++;}echo(support("comment")?"<td id='Comment-".h($A)."'>".($Gm?h(isset($P["Comment"])?$P["Comment"]:""):""):""),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: partialArg(tableClick, true)});"),"<tfoot><tr>","<td><th>".lang(275,count($gl)),"<td>".h(DIALECT=="sql"?Connection::get()->getValue("SELECT @@default_storage_engine"):""),($pc!=""?"<td>".h($pc):"");if($Gm&&function_exists('AdminNeo\db_status'))$Mk=db_status();foreach($Mk
as$t=>$Lk)echo"<td align='right' id='sum-$t'>".($Gm?format_number($Lk):"");echo"<td></td><td></td>";if(support("comment"))echo"<td></td>";echo"</tr></tfoot>\n","</table>\n","</div>\n",($Gm?"":script("ajaxSetHtml('".js_escape(ME)."script=db');"));if(Admin::get()->isDataEditAllowed()){echo"<div class='table-footer'><div class='field-sets'>\n";$lm="<input type='submit' class='button' value='".lang(303)."'> ".help_script("VACUUM");$Kh="<input type='submit' class='button' name='optimize' value='".lang(304)."'> ".help_script(DIALECT=="sql"?"OPTIMIZE TABLE":"VACUUM ANALYZE");echo"<fieldset><legend>".lang(159)." <span id='selected'></span></legend><div class='fieldset-content'>".(DIALECT=="sqlite"?$lm."<input type='submit' class='button' name='check' value='".lang(305)."'> ".help_script("PRAGMA integrity_check"):(DIALECT=="pgsql"?$lm.$Kh:(DIALECT=="sql"?"<input type='submit' class='button' value='".lang(306)."'> ".help_script("ANALYZE TABLE").$Kh."<input type='submit' class='button' name='check' value='".lang(305)."'> ".help_script("CHECK TABLE")."<input type='submit' class='button' name='repair' value='".lang(307)."'> ".help_script("REPAIR TABLE"):"")))."<input type='submit' class='button' name='truncate' value='".lang(308)."'> ".help_script(DIALECT=="sqlite"?"DELETE":("TRUNCATE".(DIALECT=="pgsql"?"":" TABLE"))).confirm().(DIALECT=="pgsql"?"<input type='submit' class='button' name='truncate_cascade' value='".lang(309)."'> ".help_script("TRUNCATE CASCADE").confirm():"")."<input type='submit' class='button' name='drop' value='".lang(160)."'>".help_script("DROP TABLE").confirm()."\n";$f=(support("scheme")?Admin::get()->getSchemas():Admin::get()->getDatabases());echo"</div></fieldset>\n";$Oj="";if(count($f)!=1&&DIALECT!="sqlite"){echo"<fieldset><legend>".lang(310)." <span id='selected3'></span></legend><div>";$g=(isset($_POST["target"])?$_POST["target"]:(support("scheme")?$_GET["ns"]:DB));echo($f?html_select("target",$f,$g,"","label-move"):'<input class="input" name="target" value="'.h($g).'" autocapitalize="off">')," <input type='submit' class='button' name='move' value='".lang(311)."'>",(support("copy")?" <input type='submit' class='button' name='copy' value='".lang(312)."'> ".checkbox("overwrite",1,$_POST["overwrite"],lang(313)):""),"</div></fieldset>\n";$Oj=" selectCount('selected3', formChecked(this, /^(tables|views)\[/));";}echo
input_hidden("all"),script("qsl('input').onclick = function () { selectCount('selected', formChecked(this, /^(tables|views)\[/));".(support("table")?" selectCount('selected2', formChecked(this, /^tables\[/) || $S);":"")."$Oj }"),input_token(),"</div></div>\n",script("initTableFooter()");}echo"</div>\n","</form>\n",script("tableCheck();");}echo'<p class="links"><a href="',h(ME),'create=">',icon("table-add"),lang(77),"</a>\n";if(support("view"))echo'<a href="',h(ME),'view=">',icon("view-add"),lang(237),"</a>\n";if(support("routine")){echo"<h2 id='routines'>".lang(178)."</h2>\n";$Bj=routines();if($Bj){$Mb=$Bj[0]["ROUTINE_COMMENT"]!==null;echo"<table>\n",'<thead><tr>','<th>',lang(217),'</th><td>',lang(44),'</td><td>',lang(254),"</td>";if($Mb)echo"<td>",lang(46),"</td>";echo"<td></td>","</tr></thead>\n";foreach($Bj
as$K){$A=($K["SPECIFIC_NAME"]==$K["ROUTINE_NAME"]?"":"&name=".urlencode($K["ROUTINE_NAME"]));echo'<tr>','<th><a href="',h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'callf=':'call=').urlencode($K["SPECIFIC_NAME"]).$A),'">',h($K["ROUTINE_NAME"]),'</a></th>','<td>',h($K["ROUTINE_TYPE"]),'</td>','<td>',h($K["DTD_IDENTIFIER"]),'</td>';if($Mb)echo'<td>',truncate_utf8(preg_replace('~\s{2,}~'," ",trim($K["ROUTINE_COMMENT"])),50),'</td>';echo'<td><a href="'.h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'function=':'procedure=').urlencode($K["SPECIFIC_NAME"]).$A).'">'.lang(170)."</a></td>";}echo"</table>\n";}echo'<p class="links">';if(support("procedure"))echo'<a href="',h(ME),'procedure=">',icon("function-add"),lang(253),"</a>";echo'<a href="',h(ME),'function=">',icon("function-add"),lang(252),"</a>\n","</p>\n";}if(support("event")){echo"<h2 id='events'>".lang(179)."</h2>\n";$L=get_rows("SHOW EVENTS");if($L){echo"<table>\n","<thead><tr><th>".lang(217)."<td>".lang(314)."<td>".lang(243)."<td>".lang(244)."<td></thead>\n";foreach($L
as$K)echo"<tr>","<th>".h($K["Name"]),"<td>".($K["Execute at"]?lang(315)."<td>".h($K["Execute at"]):lang(245)." ".h($K["Interval value"])." ".h($K["Interval field"])."<td>".h($K["Starts"])),"<td>".h($K["Ends"]),'<td><a href="'.h(ME).'event='.urlencode($K["Name"]).'">'.lang(170).'</a>';echo"</table>\n";$td=Connection::get()->getValue("SELECT @@event_scheduler");if($td&&$td!="ON")echo"<p class='error'><code class='jush-sqlset'>event_scheduler</code>: ".h($td)."\n";}echo'<p class="links"><a href="',h(ME),'event=">',icon("event-add"),lang(242),"</a></p>\n";}}}page_footer();