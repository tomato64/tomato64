<?php
/**
 * AdminNeo - Powerful database manager in a single PHP file
 * v5.8.0
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
inject($ya,Config$Tb,Settings$O,Locale$yg){$this->admin=$ya;$this->config=$Tb;$this->settings=$O;$this->locale=$yg;}}abstract
class
Origin
extends
Plugin{private$errors=[];private
static$instance=null;static
function
create(array$Tb=[],array$Oi=[]){if(self::$instance)die("Admin instance already exists.\n");$ya=new
static();if(!$Tb&&file_exists("adminneo-config.php")){$Tb=include_once("adminneo-config.php");if(!is_array($Tb)){$Tb=[];$qg="href=https://github.com/adminneo-org/adminneo#configuration ".target_blank();$ya->addError(lang(0,"<b>adminneo-config.php</b>")." <a $qg>".lang(1)."</a>");}}$Tb=new
Config($Tb);$O=new
Settings($Tb);if(!$Oi&&file_exists("adminneo-plugins.php")){$Oi=include_once("adminneo-plugins.php");if(!is_array($Oi)){$Oi=[];$qg="href=https://github.com/adminneo-org/adminneo#plugins ".target_blank();$ya->addError(lang(0,"<b>adminneo-plugins.php</b>")." <a $qg>".lang(1)."</a>");}}self::$instance=$Oi?new
Pluginer($ya,$Oi):$ya;$ya->inject(self::$instance,$Tb,$O,Locale::get());foreach($Oi
as$Ni)$Ni->inject(self::$instance,$Tb,$O,Locale::get());return
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
verifyDefaultPassword($F){$Je=$this->config->getDefaultPasswordHash();if($Je===null||$Je==="")return
lang(2);elseif(!password_verify($F,$Je))return
lang(3);return
true;}function
authenticate($V,$F){if($F==""){$Je=$this->config->getDefaultPasswordHash();if($Je===null)return
lang(4,target_blank());else
return$Je==="";}return
true;}function
getPrivateKey($ec=false){return
get_private_key($ec);}function
getBruteForceKey(){return$_SERVER["REMOTE_ADDR"];}function
getServerName($N,$Ej=true,$Kd=null){if($N==""){if(!$Ej)return"";$N=Connection::exists()?Connection::get()->getDefaultServerName():"";if($N=="")return$Kd!==null?$Kd:lang(5);$ok=null;}else$ok=$this->config->getServer($N);return$ok?$ok->getName():preg_replace('~^https?://~',"",$N);}abstract
function
getDatabase();function
getDatabases($de=true){$f=$this->filterListWithWildcards(get_databases($de),$this->config->getHiddenDatabases(),false,Driver::get()->getSystemDatabases());if(DB!=""&&!in_array(DB,$f))array_unshift($f,DB);return$f;}function
getSchemas($wh=false){$Oe=$this->config->getHiddenSchemas();if($wh&&!in_array("__system",$Oe))$Oe[]="__system";$Zj=$this->filterListWithWildcards(schemas(),$Oe,false,Driver::get()->getSystemSchemas());if(isset($_GET["ns"])&&$_GET["ns"]!=""&&!in_array($_GET["ns"],$Zj))array_unshift($Zj,$_GET["ns"]);return$Zj;}function
getCollations(array$Mf=[]){$Lm=$this->config->getVisibleCollations();$Xd=$Lm?array_merge($Lm,$Mf):[];return$this->filterListWithWildcards(collations(),$Xd,true);}private
function
filterListWithWildcards(array$Cm,array$Xd,$Of,array$hl=[]){if(!$Cm||!$Xd)return$Cm;$r=array_search("__system",$Xd);if($r!==false){unset($Xd[$r]);$Xd=array_merge($Xd,$hl);}array_walk($Xd,function(&$Y){$Y=str_replace('\\*',".*",preg_quote($Y,"~"));});$Hi='~^('.implode("|",$Xd).')$~';return$this->filterListWithPattern($Cm,$Hi,$Of);}private
function
filterListWithPattern(array$Cm,$Hi,$Of){$I=[];foreach($Cm
as$t=>$Y){if(is_array($Y)){if($Xk=$this->filterListWithPattern($Y,$Hi,$Of))$I[$t]=$Xk;}elseif(($Of&&preg_match($Hi,$Y))||(!$Of&&!preg_match($Hi,$Y)))$I[$t]=$Y;}return$I;}abstract
function
getQueryTimeout();function
sendHeaders(){}function
updateCspHeader(array&$ic){}function
printFavicons(){$Db=validate_color_variant($this->config->getColorVariant());echo"<link rel='icon' type='image/x-icon' href='",link_files("favicon-$Db.ico",[]),"' sizes='32x32'>\n","<link rel='icon' type='image/svg+xml' href='",link_files("favicon-$Db.svg",[]),"'>\n","<link rel='apple-touch-icon' href='",link_files("apple-touch-icon-$Db.png",[]),"'>\n";}abstract
function
printToHead();function
getCssUrls(){$sm=$this->config->getCssUrls();foreach(["adminneo.css","adminneo-light.css","adminneo-dark.css"]as$m){if(file_exists($m))$sm[]="$m?v=".filemtime($m);}return$sm;}function
isLightModeForced(){return$this->isColorSchemeForced(false);}function
isDarkModeForced(){return$this->isColorSchemeForced(true);}private
function
isColorSchemeForced($nc){$bh=$nc?Settings::$ColorSchemeDark:Settings::$ColorSchemeLight;$ch=$nc?Settings::$ColorSchemeLight:Settings::$ColorSchemeDark;$Td=file_exists("adminneo-$bh.css");$Ud=file_exists("adminneo-$ch.css");if($Td&&!$Ud)return
true;return$this->settings->getColorScheme()==$bh&&!($Td
xor$Ud);}function
getJsUrls(){$sm=$this->config->getJsUrls();$m="adminneo.js";if(file_exists($m))$sm[]="$m?v=".filemtime($m);return$sm;}abstract
function
printLoginForm();function
getLoginFormRow($Od,$Wf,$j){if($Wf)return"<tr><th>$Wf</th><td>$j</td></tr>\n";else
return"$j\n";}function
printLogout(){echo"<div class='logout'>","<form action='' method='post'>\n","<span title='",lang(6),"'>",h($_GET["username"]),"</span>","<input type='submit' class='button' name='logout' value='",lang(7),"' id='logout'>",input_token(),"</form>","</div>\n";}function
getTableName(array$ll){return
h($ll["Name"]);}abstract
function
getFieldName(array$j,$D=0);function
formatComment($Lb){return
h($Lb);}abstract
function
printTableMenu(array$ll,$rf);function
getForeignKeys($Q){return
foreign_keys($Q);}function
getBackwardKeys($Q,$jl){if(!$this->settings->isRelationLinks())return[];$L=backward_keys($Q);$Qf=[];foreach($L
as$K){$q=$K["table_schema"].".".$K["table_name"];$Qf[$q]["schema"]=$K["table_schema"];$Qf[$q]["table"]=$K["table_name"];$Qf[$q]["constraints"][$K["constraint_name"]][$K["column_name"]]=$K["referenced_column_name"];}foreach($Qf
as$q=>$t){$A=$this->admin->getTableName(table_status1($t["table"],true));if($A!=""){$bk=preg_quote($jl);$lk="(:|\\s*-)?\\s+";$Qf[$q]["name"]=(preg_match("(^$bk$lk(.+)|^(.+?)$lk$bk\$)iu",$A,$y)?$y[2].$y[3]:$A);}else
unset($Qf[$q]);}return$Qf;}function
printBackwardKeys(array$Ua,array$K){foreach($Ua
as$t){foreach($t["constraints"]as$Wb){$Ng=preg_replace('~&ns=[^&]+&~',"&ns=".urldecode($t["schema"])."&",ME);$w=$Ng.'select='.urlencode($t["table"]);$p=0;foreach($Wb
as$b=>$X){if(!isset($K[$X]))continue
2;$w
.=where_link($p++,$b,$K[$X]);}$A=preg_replace('(^'.preg_quote($_GET["select"]).(substr($_GET["select"],-1)=="s"?"?":"").'_)',"_",$t["name"]);$T=implode(", ",array_keys($Wb));echo"<a href='".h($w)."' title='".h($T)."'>".h($A)."</a>";$w=$Ng.'edit='.urlencode($t["table"]);foreach($Wb
as$b=>$X)$w
.="&preset".urlencode("[".bracket_escape($b)."]")."=".urlencode($K[$X]);echo"<a href='".h($w)."' title='".lang(8)."'>",icon_solo("add"),"</a> ";}}}abstract
function
formatSelectQuery($H,$Pk,$Jd=false);abstract
function
formatMessageQuery($H,$Jl,$Jd=false);abstract
function
formatSqlCommandQuery($H);function
printAfterSqlCommand(){}abstract
function
getTableDescriptionFieldName($Q);abstract
function
fillForeignDescriptions(array$L,array$ge);function
getFieldValueLink($X,$j){if(is_mail($X))return"mailto:$X";if(is_web_url($X))return$X;return
null;}abstract
function
formatSelectionValue($X,$w,$j,$li);abstract
function
formatFieldValue($Y,array$j);abstract
function
printTableStructure(array$k);abstract
function
printTablePartitions(array$yi);abstract
function
printRelatedTables(array$S);abstract
function
printTableIndexes(array$s,array$ll);abstract
function
printSelectionColumns(array$M,array$c);abstract
function
printSelectionSearch(array$Z,array$c,array$s);abstract
function
printSelectionOrder(array$D,array$c,array$s);abstract
function
printSelectionLimit($v);abstract
function
printSelectionLength($El);abstract
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
detectJson($Pd,&$Y,$Zi=null){if(is_array($Y)){$be=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($this->config->isJsonValuesAutoFormat()?JSON_PRETTY_PRINT:0);$Y=json_encode($Y,$be);return
true;}$be=JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES|($Zi?JSON_PRETTY_PRINT:0);if(preg_match('~^jsonb?$~',$Pd)){if($Y!=null&&$Zi!==null&&$this->config->isJsonValuesAutoFormat())$Y=json_encode(json_decode($Y),$be);return
true;}if(!$this->config->isJsonValuesDetection())return
false;if(is_string($Y)&&$Y!=""&&preg_match('~varchar|text|character varying|String|keyword~',$Pd)&&($Y[0]=="{"||$Y[0]=="[")&&($Kf=json_decode($Y))){if($Zi!==null&&$this->config->isJsonValuesAutoFormat())$Y=json_encode($Kf,$be);return
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
sendDumpHeaders($Ze,$fh=false);function
dumpDatabase($qc){}abstract
function
dumpTable($Q,$Wk,$Im=0);abstract
function
dumpData($Q,$Wk,$H);abstract
function
getImportFilePath();abstract
function
printDatabaseMenu();abstract
function
printNavigation($Zg);abstract
function
printDatabaseSwitcher($Zg);function
printTablesFilter(){echo"<div class='tables-filter jsonly'>"."<input id='tables-filter' type='search' class='input' autocomplete='off' placeholder='".lang(9)."'>".script("initTablesFilter(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");")."</div>\n";}abstract
function
printTableList(array$S);function
getSettingsRows($Be){$O=[];if($Be==1){$C=get_language_options();if($C)$O["lang"]="<tr><th id='label-language'>".lang(10)."</th>"."<td>".html_select("lang",get_language_options(),Locale::get()->getLanguage(),"","label-language")."</td></tr>\n";$Ol=get_theme_titles($this->config->getColorVariant());if(count($Ol)>1){list($Gl)=validate_theme($this->config->getTheme(),$this->config->getColorVariant());$C=[""=>lang(11)." ($Ol[$Gl])"]+$Ol;$O["theme"]="<tr><th id='label-theme'>".lang(12)."</th>"."<td>".html_select("theme",$C,($ra=$this->settings->getParameter("theme"))!==null?$ra:"","","label-theme")."</td></tr>\n";}$C=[""=>lang(13),Settings::$ColorSchemeLight=>lang(14),Settings::$ColorSchemeDark=>lang(15)];$O["colorScheme"]="<tr><th>".lang(16)."</th>"."<td>".html_radios("colorScheme",$C,($ra=$this->settings->getParameter("colorScheme"))!==null?$ra:"")."</td></tr>\n";}elseif($Be==2){$C=[""=>lang(11),true=>lang(17),false=>lang(18),];$h=$C[$this->config->isRelationLinks()];$C[""].=" ($h)";$O["relationLinks"]="<tr><th>".lang(19)."</th>"."<td>".html_radios("relationLinks",$C,($ra=$this->settings->getParameter("relationLinks"))!==null?$ra:"")."<span class='input-hint'>".lang(20)."</span>"."</td></tr>\n";$h=$this->config->getRecordsPerPage();$C=[""=>lang(11)." ($h)","20","30","50","70","100",];$O["recordsPerPage"]="<tr><th id='label-records'>".lang(21)."</th>"."<td>".html_select("recordsPerPage",$C,($ra=$this->settings->getParameter("recordsPerPage"))!==null?$ra:"","","label-records")."<span class='input-hint'>".lang(22)."</span>"."</td></tr>\n";$h=($ra=$this->config->getEnumAsSelectThreshold())!==null?$ra:lang(23);$C=[""=>lang(11)." ($h)",-1=>lang(23),0=>lang(24),3=>lang(25,3),5=>lang(25,5),10=>lang(25,10),20=>lang(25,20),];$O["enumAsSelectThreshold"]="<tr><th id='label-enum'>".lang(26)."</th>"."<td>".html_select("enumAsSelectThreshold",$C,($ra=$this->settings->getParameter("enumAsSelectThreshold"))!==null?$ra:"","","label-enum",true)."<span class='input-hint'>".lang(27)."</span>"."</td></tr>\n";}return$O;}abstract
function
getForeignColumnInfo(array$ge,$b);}class
Pluginer{private
static$InternalMethods=["inject"=>true,"getConfig"=>true,];private
static$AppendMethods=["getErrors"=>true,"getFieldFunctions"=>true,"getDumpOutputs"=>true,"getDumpFormats"=>true,"getSettingsRows"=>true,];private$plugins;private$hooks=[];function
__construct(Origin$ya,array$Oi){$this->plugins=$Oi;foreach(get_class_methods('\AdminNeo\Origin')as$Xg){$this->hooks[$Xg]=[];if(!(isset(self::$InternalMethods[$Xg])?self::$InternalMethods[$Xg]:false)){foreach($Oi
as$Ni){if(method_exists($Ni,$Xg))$this->hooks[$Xg][]=$Ni;}}if(isset(self::$AppendMethods[$Xg])?self::$AppendMethods[$Xg]:false)array_unshift($this->hooks[$Xg],$ya);else$this->hooks[$Xg][]=$ya;}}function
getPlugins(){return$this->plugins;}function
__call($A,array$ti){$Ha=isset(self::$AppendMethods[$A])?self::$AppendMethods[$A]:false;$I=$Ha?[]:null;assert(isset($this->hooks[$A]),"Calling unknown plugin method: $A");foreach($this->hooks[$A]as$Ni){$Y=call_user_func_array([$Ni,$A],$ti);if($Y!==null){if($Ha)$I+=$Y;else
return$Y;}}return$I;}function
updateCspHeader(array&$ic){$this->__call(__FUNCTION__,[&$ic]);}function
detectJson($Pd,&$Y,$Zi=null){return$this->__call(__FUNCTION__,[$Pd,&$Y,$Zi]);}}class
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
printLoginForm(){$Vc=Drivers::getList();$pk=$this->config->getServerPairs($Vc);$N=SERVER?:$this->config->getDefaultServer();echo"<table class='box box-light'>\n";if($pk)echo$this->admin->getLoginFormRow('server',lang(5),"<select name='auth[server]'>".optionlist($pk,$N,true)."</select>");else{$Tc=DRIVER?:$this->config->getDefaultDriver($Vc);if(count($Vc)>1)echo$this->admin->getLoginFormRow('driver',lang(28),html_select("auth[driver]",$Vc,$Tc).script("initLoginDriver(qsl('select'));",""));else
echo$this->admin->getLoginFormRow('driver','',input_hidden("auth[driver]",$Tc));echo$this->admin->getLoginFormRow('server',lang(5),"<input class='input' name='auth[server]' value='".h($N)."' title='".lang(29)."' placeholder='localhost' autocapitalize='off'>");}echo$this->admin->getLoginFormRow('username',lang(6),'<input class="input" name="auth[username]" id="username" value="'.h($_GET["username"]).'" autocomplete="username" autocapitalize="off">'),$this->admin->getLoginFormRow('password',lang(30),'<input type="password" class="input" name="auth[password]" autocomplete="current-password">');if(!$pk){$qc=isset($_GET["db"])?$_GET["db"]:$this->config->getDefaultDatabase();echo$this->admin->getLoginFormRow('db',lang(31),'<input class="input" name="auth[db]" value="'.h($qc).'" autocapitalize="off">');}echo"</table>\n","<p>","<input type='submit' class='button default' value='".lang(32)."'>",checkbox("auth[permanent]",1,$_COOKIE["neo_permanent"],lang(33)),"</p>\n";}function
getFieldName(array$j,$D=0){$U=$j["full_type"].($j["null"]?" NULL":"");$Lb=$j["comment"];$lk=$U&&$Lb!=""?": ":"";return'<span title="'.h($U.$lk.$Lb).'">'.h($j["field"]).'</span>';}function
printTableMenu(array$ll,$rf){echo'<p class="links top-tabs">';$rg=[];$hk=($this->settings->isSelectionPreferred()&&!$this->settings->isNavigationReversed())||(!$this->settings->isSelectionPreferred()&&$this->settings->isNavigationReversed());if($hk)$rg["select"]=[lang(34),"data"];if(support("table")||support("indexes"))$rg["table"]=[lang(35),"structure"];if(!$hk)$rg["select"]=[lang(34),"data"];$Q=$ll["Name"];$Ff=false;if(support("table")){$Ff=is_view($ll);if(!$Ff){if($Q!="")$rg["create"]=[lang(36),"edit"];}elseif(support("view"))$rg["view"]=[lang(37),"edit"];}if($rf!==null)$rg["edit"]=[lang(8),"item-add"];$ti=$rf?"&".http_build_query($rf):"";foreach($rg
as$t=>$X)echo" <a href='",h(ME),"$t=",urlencode($Q),($t=="edit"?$ti:""),"'",bold(isset($_GET[$t])),">",icon($X[1]),"$X[0]</a>";echo
doc_link([DIALECT=>Driver::get()->tableHelp($Q,$Ff)],icon("help").lang(38)),"\n";}function
formatSelectQuery($H,$Pk,$Jd=false){$cl=support("sql");$Pm=!$Jd?Driver::get()->warnings():null;if($cl)$H
.=";";$fl=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J="<pre><code class='jush-$fl'>".h(str_replace("\n"," ",$H))."</code></pre>\n";$J
.="<p class='links'>";if($cl)$J
.="<a href='".h(ME)."sql=".urlencode($H)."'>".icon("edit").lang(39)."</a>";if($Pm)$J
.="<a href='#warnings' class='toggle'>".lang(40).icon_chevron_down()."</a>";$J
.=" <span class='time'>(".format_time($Pk).")</span>";$J
.="</p>\n";if($Pm){$J
.=script("initToggles(qsl('p'));");$J
.="<div id='warnings' class='warnings hidden'>\n$Pm\n</div>\n";}return$J;}function
formatMessageQuery($H,$Jl,$Jd=false){restart_session();$Qe=&get_session("queries");if(!isset($Qe[$_GET["db"]]))$Qe[$_GET["db"]]=[];if(strlen($H)>1e6)$H=preg_replace('~[\x80-\xFF]+$~','',substr($H,0,1e6))."\n…";$Qe[$_GET["db"]][]=[$H,time(),$Jl];$cl=support("sql");$Pm=!$Jd?Driver::get()->warnings():null;$Lk="sql-".count($Qe[$_GET["db"]]);$Qm="warnings-".count($Qe[$_GET["db"]]);$J=" ";if($Pm)$J
.="<a href='#$Qm' class='toggle'>".lang(40).icon_chevron_down()."</a>, ";$lj=support("sql")?lang(41):lang(42);$J
.="<a href='#$Lk' class='toggle'>$lj".icon_chevron_down()."</a>";$J
.=" <span class='time'>".@date("H:i:s")."</span>\n";if($Pm)$J
.="<div id='$Qm' class='warnings hidden'>\n$Pm</div>\n";$J
.="<div id='$Lk' class='hidden'>\n";$fl=DIALECT=="elastic"||DIALECT=="mongo"?"json":DIALECT;$J
.="<pre><code class='jush-$fl'>".truncate_utf8($H,1000)."</code></pre>\n";$J
.="<p class='links'>";if($cl)$J
.="<a href='".h(str_replace("db=".urlencode(DB),"db=".urlencode($_GET["db"]),ME).'sql=&history='.(count($Qe[$_GET["db"]])-1))."'>".icon("edit").lang(39)."</a>";if($Jl)$J
.=" <span class='time'>($Jl)</span>";$J
.="</p>\n";$J
.="</div>\n";return$J;}function
formatSqlCommandQuery($H){if(preg_match('~^DELIMITER\s~i',$H))return"";return
truncate_utf8($H,1000);}function
getTableDescriptionFieldName($Q){return"";}function
fillForeignDescriptions(array$L,array$ge){return$L;}function
formatSelectionValue($X,$w,$j,$li){if($X===null)$Dl="<i>NULL</i>";elseif(!$j)$Dl=$X;elseif(preg_match("~char|binary|boolean~",$j["type"])&&!preg_match("~var~",$j["type"]))$Dl="<code>$X</code>";elseif(is_blob($j)&&!is_utf8($X))$Dl="<i>".lang(43,strlen($li))."</i>";elseif($this->admin->detectJson($j["full_type"],$li))$Dl="<code class='jush-json'>$X</code>";else$Dl=$X;if($w)$Dl="<a href='".h($w)."'".(is_web_url($w)?target_blank():"").">$Dl</a>";return$Dl;}function
formatFieldValue($Y,array$j){return$Y;}function
printTableStructure(array$k){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th>",lang(44),"</th>","<td>",lang(45),"</td>","<td>",lang(46),"</td>";if(support("comment"))echo"<td>",lang(47),"</td>";echo"</tr></thead>\n";$ym=Driver::get()->getUserTypes();foreach($k
as$j){echo"<tr>","<th>",h($j["field"]),"</th>","<td>";$U=h($j["full_type"]);if(in_array($U,$ym))echo"<a href='".h(ME.'type='.urlencode($U))."'>$U</a>";else
echo$U;if($j["null"])echo" <i>NULL</i>";if($j["auto_increment"])echo" <i>".lang(48)."</i>";$h=h($j["default"]);if(isset($j["default"]))echo" <span title='".lang(49)."'>[<b>",$j["generated"]?"<code class='jush-".DIALECT."'>$h</code>":$h,"</b>]</span>";echo"</td>","<td>",h($j["collation"]),"</td>";if(support("comment"))echo"<td>",$this->admin->formatComment($j["comment"]),"</td>";echo"\n";}echo"</table>\n","</div>\n";}function
printTablePartitions(array$yi){$zk=isset($yi["partition_names"]);echo"<p>","<code class='jush-".DIALECT."'>BY {$yi["partition_by"]} ({$yi["partition"]})</code>";if(!$zk&&isset($yi["partitions"]))echo" ".lang(50).": ".h($yi["partitions"]);echo"</p>";if($zk){echo"<table>\n","<thead><tr><th>".lang(51)."</th><td>".lang(52)."</td></tr></thead>\n";foreach($yi["partition_names"]as$t=>$A){echo"<tr><th>";if(DIALECT=="pgsql")echo"<a href='",h(ME."table=".urlencode($A)),"'>";echo
h($A);if(DIALECT=="pgsql")echo"</a>";echo"</th><td>".h($yi["partition_values"][$t])."\n";}echo"</table>\n";}}function
printRelatedTables(array$S){echo"<ul class='links'>\n";foreach($S
as$K){$w=preg_replace('~ns=[^&]*~',"ns=".urlencode($K["ns"]),ME);echo"<li><a href='",h($w."table=".urlencode($K["table"])),"'>",icon("structure");if($K["ns"]!=$_GET["ns"])echo"<b>".h($K["ns"])."</b>.";echo
h($K["table"]),"</a>";}echo"</ul>\n";}function
printTableIndexes(array$s,array$ll){$vc=first(Driver::get()->getIndexAlgorithms($ll));$wi=false;foreach($s
as$r){if(isset($r["partial"])?$r["partial"]:false){$wi=true;break;}}echo"<table>\n","<thead><tr>","<th>",lang(45),"</th>","<td>",lang(53)," (",lang(54),")</td>";if($wi)echo"<td>",lang(55),"</td>";echo"</tr></thead>\n";foreach($s
as$A=>$r){ksort($r["columns"]);$bj=[];foreach($r["columns"]as$t=>$X)$bj[]="<i>".h($X)."</i>".($r["lengths"][$t]?"(".h($r["lengths"][$t]).")":"").($r["descs"][$t]?" DESC":"");echo"<tr title='",h($A),"'>","<th>",h($r["type"]);if(isset($r['algorithm'])&&$r['algorithm']!=$vc)echo" (",h($r['algorithm']),")";echo"</th>","<td>",implode(", ",$bj),"</td>";if($wi){echo"<td>";if($r['partial'])echo"<code class='jush-",DIALECT,"'>WHERE ",h($r['partial']),"</code>";echo"</td>";}echo"</tr>\n";}echo"</table>\n";}function
printSelectionColumns(array$M,array$c){print_fieldset_start("select",lang(56),"columns",(bool)$M,true);$M[""]=[];$p=0;foreach($M
as$t=>$X){$X=isset($_GET["columns"][$t])?$_GET["columns"][$t]:[];$b=select_input("name='columns[$p][col]'",$c,isset($X["col"])?$X["col"]:null,$t!==""?"selectFieldChange":"selectAddRow");echo"<div ",($t!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly");if(Driver::get()->getFunctions()||Driver::get()->getGrouping())echo
html_select("columns[$p][fun]",[-1=>""]+array_filter([lang(57)=>Driver::get()->getFunctions(),lang(58)=>Driver::get()->getGrouping()]),isset($X["fun"])?$X["fun"]:null),help_script_command("value && value.replace(/ |\$/, '(') + ')'",true),script("qsl('select').onchange = (event) => { ".($t!==""?"":" qsl('select, input:not(.remove)', event.target.parentNode).onchange();")." };",""),"($b)";else
echo$b;echo" <button class='button light remove jsonly' title='",lang(59),"'>",icon_solo("remove"),"</button>",script("qsl('#fieldset-select .remove').onclick = selectRemoveRow;",""),"</div>\n";$p++;}print_fieldset_end("select",true);}function
printSelectionSearch(array$Z,array$c,array$s){print_fieldset_start("search",lang(60),"search",(bool)$Z);foreach($s
as$p=>$r){if($r["type"]=="FULLTEXT"){echo"<div>(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$r["columns"]))."</i>) AGAINST","<input type='text' class='input' name='fulltext[$p]' value='".h(isset($_GET["fulltext"][$p])?$_GET["fulltext"][$p]:null)."'>",script("qsl('input').oninput = selectFieldChange;","");if(DIALECT=='sql')echo
checkbox("boolean[$p]",1,isset($_GET["boolean"][$p]),"BOOL");echo"</div>\n";}}$mb="this.parentNode.firstChild.onchange();";foreach(array_merge((array)$_GET["where"],[[]])as$p=>$X){if(!$X||("$X[col]$X[val]"!=""&&in_array($X["op"],$this->getOperators())))echo"<div>",select_input(" name='where[$p][col]'",$c,$X["col"],($X?"selectFieldChange":"selectAddRow"),"(".lang(61).")"),html_select("where[$p][op]",$this->getOperators(),$X["op"],$mb),"<input type='text' class='input' name='where[$p][val]' value='".h($X["val"])."'>",script("mixin(qsl('input'), {oninput: function () { $mb }, onkeydown: selectSearchKeydown});","")," <button class='button light remove jsonly' title='".lang(59)."'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-search .remove").onclick = selectRemoveRow;',""),"</div>\n";}print_fieldset_end("search");}function
printSelectionOrder(array$D,array$c,array$s){print_fieldset_start("sort",lang(62),"sort",(bool)$D,true);$_GET["order"][""]="";$p=0;foreach((array)$_GET["order"]as$t=>$X){if($t!=""&&$X=="")continue;echo"<div ",($t!=""?"":"class='no-sort'"),">",icon("handle","handle jsonly"),select_input("name='order[$p]'",$c,$X,$t!==""?"selectFieldChange":"selectAddRow")," ",checkbox("desc[$p]",1,isset($_GET["desc"][$t]),lang(63))," <button class='button light remove jsonly' title='",lang(59),"'>",icon_solo("remove"),"</button>",script('qsl("#fieldset-sort .remove").onclick = selectRemoveRow;',""),"</div>\n";$p++;}print_fieldset_end("sort",true);}function
printSelectionLimit($v){echo"<fieldset><legend>".lang(64)."</legend><div class='fieldset-content'>","<input type='number' name='limit' class='input size' value='$v'>",script("qsl('input').oninput = selectFieldChange;",""),"</div></fieldset>\n";}function
printSelectionLength($El){if($El!==null)echo"<fieldset><legend>".lang(65)."</legend><div class='fieldset-content'>","<input type='number' name='text_length' class='input size' value='".h($El)."'>","</div></fieldset>\n";}function
printSelectionAction(array$s){echo"<fieldset><legend>".lang(66)."</legend><div class='fieldset-content'>","<input type='submit' class='button' value='".lang(56)."'>"," <span id='noindex' title='".lang(67)."'></span>","<script".nonce().">\n";$c=new
stdClass();foreach($s
as$r){$kc=reset($r["columns"]);if($r["type"]!="FULLTEXT"&&$kc)$c->$kc=null;}echo"const indexColumns = ".json_encode($c,JSON_UNESCAPED_UNICODE|JSON_HEX_TAG).";\n","selectFieldChange.call(gid('form')['select']);\n","</script>\n","</div></fieldset>\n";}function
processSelectionColumns(array$c,array$s){$M=[];$_e=[];foreach((array)$_GET["columns"]as$t=>$X){if($X["fun"]=="count"||($X["col"]!=""&&(!$X["fun"]||in_array($X["fun"],Driver::get()->getFunctions())||in_array($X["fun"],Driver::get()->getGrouping())))){$M[$t]=apply_sql_function($X["fun"],($X["col"]!=""?idf_escape($X["col"]):"*"));if(!in_array($X["fun"],Driver::get()->getGrouping()))$_e[]=$M[$t];}}return[$M,$_e];}function
processSelectionSearch(array$k,array$s){$J=[];foreach($s
as$p=>$r){if($r["type"]=="FULLTEXT"&&isset($_GET["fulltext"])&&$_GET["fulltext"][$p]!="")$J[]="MATCH (".implode(", ",array_map('AdminNeo\idf_escape',$r["columns"])).") AGAINST (".q($_GET["fulltext"][$p]).(isset($_GET["boolean"][$p])?" IN BOOLEAN MODE":"").")";}foreach((array)$_GET["where"]as$Z){$_b=$Z["col"];$Ph=$Z["op"];$X=$Z["val"];if("$_b$X"!=""&&in_array($Ph,$this->getOperators())){$Sb=[];foreach(($_b!=""?[$_b=>$k[$_b]]:$k)as$A=>$j){$Xi="";$Rb=" $Ph";$Eh=DIALECT=="pgsql"&&$Ph=="="&&$j["type"]=="oid";if($Eh)$Rb
.=" ".$this->admin->processFieldInput($j,$X)."::regproc";elseif(preg_match('~IN$~',$Ph)){$ef=process_length($X);$Rb
.=" ".($ef!=""?$ef:"(NULL)");}elseif($Ph=="SQL")$Rb=" $X";elseif(preg_match('~^(I?LIKE) %%$~',$Ph,$y))$Rb=" $y[1] ".$this->admin->processFieldInput($j,"%$X%");elseif($Ph=="FIND_IN_SET"){$Xi="$Ph(".q($X).", ";$Rb=")";}elseif(!preg_match('~NULL$~',$Ph))$Rb
.=" ".$this->admin->processFieldInput($j,$X);if($_b!=""||(isset($j["privileges"]["where"])&&(preg_match('~^[-\d.'.(preg_match('~IN$~',$Ph)?',':'').']+$~',$X)||!preg_match('~'.number_type().'|bit~',$j["type"]))&&(!preg_match("~[\x80-\xFF]~",$X)||preg_match('~char|text|enum|set~',$j["type"]))&&(!preg_match('~date|timestamp~',$j["type"])||preg_match('~^\d+-\d+-\d+~',$X))&&(!preg_match('~^elastic~',DRIVER)||$j["type"]!="boolean"||preg_match('~true|false~',$X))&&(!preg_match('~^elastic~',DRIVER)||strpos($Ph,"regexp")===false||preg_match('~text|keyword~',$j["type"])))){if($Eh)$Sb[]=$Xi.idf_escape($A).$Rb;else$Sb[]=$Xi.Driver::get()->convertSearch(idf_escape($A),$Z,$j).$Rb;}}if(count($Sb)==1)$J[]=$Sb[0];elseif($Sb)$J[]="(".implode(" OR ",$Sb).")";else$J[]="1 = 0";}}return$J;}function
processSelectionOrder(array$k,array$s){$J=[];foreach((array)$_GET["order"]as$t=>$X){if($X!="")$J[]=(preg_match('~^((COUNT\(DISTINCT |[A-Z0-9_]+\()(`(?:[^`]|``)+`|"(?:[^"]|"")+")\)|COUNT\(\*\))$~',$X)?$X:idf_escape($X)).(isset($_GET["desc"][$t])?" DESC".(DIALECT=="pgsql"&&(isset($k[$X]["null"])?$k[$X]["null"]:null)?" NULLS LAST":""):"");}return$J;}function
processSelectionLength(){return
isset($_GET["text_length"])?$_GET["text_length"]:"100";}function
getFieldFunctions(array$j){$J=($j["null"]?"NULL/":"");$pm=isset($_GET["select"])||where($_GET);foreach([Driver::get()->getInsertFunctions(),Driver::get()->getEditFunctions()]as$t=>$se){if(!$t||(!isset($_GET["call"])&&$pm)){foreach($se
as$Hi=>$X){if(!$Hi||preg_match("~$Hi~",$j["type"]))$J
.="/$X";}}if($t&&$se&&!preg_match('~enum|set|bool~',$j["type"])&&!is_blob($j))$J
.="/SQL";}if($j["auto_increment"]&&!$pm)$J=lang(48);return
explode("/",$J);}function
getFieldInput($Q,array$j,$Ma,$Y,$o){return"";}function
processFieldInput(array$j,$Y,$o=""){if($o=="SQL")return$Y;if(isset($j["full_type"]))$this->admin->detectJson($j["full_type"],$Y,false);$A=$j["field"];$J=q($Y);if(preg_match('~^(now|getdate|uuid)$~',$o))$J="$o()";elseif(preg_match('~^current_(date|timestamp)$~',$o))$J=$o;elseif(preg_match('~^([+-]|\|\|)$~',$o))$J=idf_escape($A)." $o $J";elseif(preg_match('~^[+-] interval$~',$o))$J=idf_escape($A)." $o ".(preg_match("~^(\\d+|'[0-9.: -]') [A-Z_]+\$~i",$Y)&&DIALECT!="pgsql"?$Y:$J);elseif(preg_match('~^(addtime|subtime|concat)$~',$o))$J="$o(".idf_escape($A).", $J)";elseif(preg_match('~^(md5|sha1|password|encrypt)$~',$o))$J="$o($J)";elseif($j["type"]=="boolean"&&DIALECT=="elastic")$J=$J=="0"?"false":"true";return
unconvert_field($j,$J);}function
getDumpOutputs(){$pi=['file'=>lang(68),'text'=>lang(69),];if(function_exists('gzencode'))$pi['gz']='gzip';return$pi;}function
getDumpFormats(){return(support("dump")?['sql'=>'SQL']:[])+['csv'=>'CSV,','csv;'=>'CSV;','tsv'=>'TSV'];}function
sendDumpHeaders($Ze,$fh=false){$oi=$_POST["output"];$Fd=(str_contains($_POST["format"],"sql")?"sql":($fh?"tar":"csv"));if($oi=="gz"){header("Content-Type: application/x-gzip");ob_start(function($Tk){return
gzencode($Tk);},1e6);}elseif($Fd=="tar")header("Content-Type: application/x-tar");elseif($Fd=="sql"||$oi=="text")header("Content-Type: text/plain; charset=utf-8");else
header("Content-Type: text/csv; charset=utf-8");return$Fd;}function
dumpTable($Q,$Wk,$Im=0){if($_POST["format"]!="sql"){echo"\xef\xbb\xbf";if($Wk)dump_csv(array_keys(fields($Q)));}else{if($Im==2){$k=[];foreach(fields($Q)as$A=>$j)$k[]=idf_escape($A)." $j[full_type]";$ec="CREATE TABLE ".table($Q)." (".implode(", ",$k).")";}else$ec=create_sql($Q,$_POST["auto_increment"],$Wk);set_utf8mb4($ec);if($Wk&&$ec){if($Wk=="DROP+CREATE"||$Im==1)echo"DROP ".($Im==2?"VIEW":"TABLE")." IF EXISTS ".table($Q).";\n";if($Im==1)$ec=remove_definer($ec);echo"$ec;\n\n";}}}function
dumpData($Q,$Wk,$H){if($Wk){$Gg=(DIALECT=="sqlite"?0:1048576);$k=[];$af=false;if($_POST["format"]=="sql"){if($Wk=="TRUNCATE+INSERT")echo
truncate_sql($Q).";\n";$k=fields($Q);if(DIALECT=="mssql"){foreach($k
as$j){if($j["auto_increment"]){echo"SET IDENTITY_INSERT ".table($Q)." ON;\n";$af=true;break;}}}}$I=Connection::get()->query($H,1);if($I){$pf="";$eb="";$Qf=[];$ue=[];$Zk="";$dc=0;while($K=($Q!=''?$I->fetchAssoc():$I->fetchRow())){if(!$Qf){$Cm=[];foreach($K
as$X){$j=$I->fetchField();if(!empty($k[$j->name]['generated'])){$ue[$j->name]=true;continue;}$Qf[]=$j->name;$t=idf_escape($j->name);$Cm[]="$t = VALUES($t)";}$Zk=($Wk=="INSERT+UPDATE"?"\nON DUPLICATE KEY UPDATE ".implode(", ",$Cm):"").";\n";}if($_POST["format"]!="sql"){if($Wk=="table"){dump_csv($Qf);$Wk="INSERT";}dump_csv($K);}else{if(!$pf)$pf="INSERT INTO ".table($Q)." (".implode(", ",array_map('AdminNeo\idf_escape',$Qf)).") VALUES";foreach($K
as$t=>$X){if(isset($ue[$t])){unset($K[$t]);continue;}$j=$k[$t];$K[$t]=($X===null?"NULL":($X===false?0:unconvert_field($j,preg_match(number_type(),$j["type"])&&!preg_match('~\[~',$j["full_type"])&&is_numeric($X)?$X:(!is_blob($j)||is_utf8($X)?q($X):Driver::get()->quoteBinary($X)))));}$Qj=($Gg?"\n":" ")."(".implode(",\t",$K).")";if(!$eb)$eb=$pf.$Qj;elseif(DIALECT=="mssql"?$dc%1000!=0:strlen($eb)+4+strlen($Qj)+strlen($Zk)<$Gg)$eb
.=",$Qj";else{echo$eb.$Zk;$eb=$pf.$Qj;}}$dc++;}if($eb)echo$eb.$Zk;}elseif($_POST["format"]=="sql")echo"-- ".str_replace("\n"," ",Connection::get()->getError())."\n";if($af)echo"SET IDENTITY_INSERT ".table($Q)." OFF;\n";}}function
getImportFilePath(){return"adminneo.sql";}function
printDatabaseMenu(){echo"<p class='links top-links'>\n";$yh=isset($_GET["ns"])?$_GET["ns"]:null;if($yh==""&&support("database"))echo'<a href="',h(ME),'database=">',icon("edit"),lang(70),"</a>\n";if($yh!=""&&support("scheme"))echo"<a href='",h(ME),"scheme='>",icon("edit"),lang(71),"</a>\n";if($yh!=="")echo'<a href="',h(ME),'schema=">',icon("schema"),lang(72),"</a>\n";if(support("privileges"))echo"<a href='",h(ME),"privileges='>",icon("users"),lang(73),"</a>\n";echo"</p>\n";}function
printNavigation($Zg){if($Zg=="auth"){$oi="";foreach((array)$_SESSION["pwds"]as$Em=>$tk){foreach($tk
as$N=>$zm){foreach($zm
as$V=>$F){if($F!==null){$tc=$_SESSION["db"][$Em][$N][$V];foreach(($tc?array_keys($tc):[""])as$g){$qk=$this->admin->getServerName($N,false);$T=h(get_driver_name($Em,$N)).($V!=""||$qk!=""?" - ":"").h($V).($V!=""&&$qk!=""?"@":"").h($qk).($g!=""?h(" - $g"):"");$oi
.="<li><a href='".h(auth_url($Em,$N,$V,$g))."' class='primary' title='$T'>$T</a></li>\n";}}}}}if($oi)echo"<nav id='logins'><menu>\n$oi</menu></nav>\n";}else{$this->admin->printDatabaseSwitcher($Zg);$va=[];if(DB==""||!$Zg){if(support("sql")){$va[]="<a href='".h(ME)."sql='".bold(isset($_GET["sql"])&&!isset($_GET["import"])).">".icon("command").lang(41)."</a>";$va[]="<a href='".h(ME)."import='".bold(isset($_GET["import"])).">".icon("import").lang(74)."</a>";}$va[]="<a href='".h(ME)."dump=".urlencode(isset($_GET["table"])?$_GET["table"]:$_GET["select"])."' id='dump'".bold(isset($_GET["dump"])).">".icon("export").lang(75)."</a>";}if(DB=="")$va[]='<a href="'.h(ME).'database="'.bold($_GET["database"]==="").">".icon("database-add").lang(76)."</a>\n";if(DB!=""&&$_GET["ns"]===""&&!$Zg)$va[]='<a href="'.h(ME).'scheme="'.bold($_GET["scheme"]==="").">".icon("database-add").lang(77)."</a>\n";if(DB!=""&&$_GET["ns"]!==""&&!$Zg)$va[]='<a href="'.h(ME).'create="'.bold($_GET["create"]==="").">".icon("table-add").lang(78)."</a>\n";if($va)echo"<p class='links'>".implode("\n",$va)."</p>";$S=[];if($_GET["ns"]!==""&&!$Zg&&DB!=""){Connection::get()->selectDatabase(DB);$S=table_status('',true);}if($_GET["ns"]!==""&&!$Zg&&DB!=""){if($S){$this->admin->printTablesFilter();$this->admin->printTableList($S);}else
echo"<div id='tables'><p>".lang(79)."</p></div>\n";}elseif($_GET["ns"]!==""&&DB!="")echo"<div id='tables'></div>\n";if(support("sql")||DIALECT=="elastic"||DIALECT=="mongo"){echo"<script".nonce().">\n";if(support("sql")&&$S){$rg=[];foreach($S
as$Q=>$U)$rg[]=js_escape_re($Q);$kl=support("table")&&!$this->config->isSelectionPreferred()?"table":"select";echo"window.jushLinks = { ".DIALECT.": {\n",js_escape_key(ME.$kl.'=$&'),': /\b(?<!\$)('.implode('|',$rg).')(?!\$)\b/g';$Mk=["sql","check","event","procedure","trigger","view","type","table","processlist"];if(support('routine')&&array_intersect_key($_GET,array_flip($Mk))){foreach(routines()as$K)echo",\n",js_escape_key(ME.'function='.urlencode($K["SPECIFIC_NAME"]).'&name=$&'),': /\b'.js_escape_re($K["ROUTINE_NAME"]).'(?=["`\]]?\()/g';}echo"\n}};\n";foreach(["bac","bra","sqlite_quo","mssql_bra"]as$X)echo"jushLinks.$X = jushLinks.".DIALECT.";\n";}if(DIALECT!="elastic"&&DIALECT!="mongo"&&$this->getConfig()->isSqlAutocompletionEnabled()&&(isset($_GET["sql"])||isset($_GET["trigger"])||isset($_GET["check"]))){$ul=array_fill_keys(array_keys($S),[]);foreach(Driver::get()->getAllFields()as$Q=>$k){foreach($k
as$j)$ul[$Q][]=$j["field"];}echo"window.addEventListener('DOMContentLoaded', () => { autocompletion = jush.autocompleteSql('".idf_escape("")."', ".json_encode($ul,JSON_HEX_TAG)."); });\n";}echo"</script>\n";}echo
script("let autocompletion;\nwindow.addEventListener('DOMContentLoaded', () => { initSyntaxHighlighting('".js_escape(doc_version())."', '".js_escape(Connection::get()->getFlavor())."', autocompletion); });");}}function
printDatabaseSwitcher($Zg){$f=$this->admin->getDatabases();if(!$f&&DIALECT!="sqlite")return;echo"<div class='db-selector'><form action=''>";hidden_fields_get();echo"<div>";if($f)echo"<select id='database-select' name='db' title='",lang(31),"'>".optionlist([""=>"(".lang(80).")"]+$f,DB)."</select>".script("mixin(gid('database-select'), {onmousedown: dbMouseDown, onchange: dbChange});");else
echo"<input id='database-select' class='input' name='db' value='".h(DB)."' title='",lang(31),"' autocapitalize='off'>\n";echo"<input type='submit' value='".lang(81)."' class='button ".($f?"hidden":"")."'>\n","</div>";foreach(["import","sql","schema","dump","privileges"]as$X){if(isset($_GET[$X])){echo
input_hidden($X);break;}}echo"</form></div>\n";}function
printTableList(array$S){$ad=$this->settings->isNavigationDual()||$this->settings->isNavigationHover();$Pg=($ad?"class='dual".($this->settings->isNavigationHover()?" hover":"")."'":($this->settings->isNavigationReversed()?"class='reversed'":""));echo"<nav id='tables'><div class='scroll-marker'></div><menu $Pg>";foreach($S
as$Q=>$P){$Q="$Q";$A=$this->admin->getTableName($P);if($A==""||(isset($P["Partition"])?$P["Partition"]:false))continue;echo"<li>";$wa=in_array($Q,[$_GET["table"],$_GET["select"],$_GET["create"],$_GET["indexes"],$_GET["foreign"],$_GET["trigger"],$_GET["check"],$_GET["view"]]);$yb="primary".(is_view($P)?" view":"");$dl=support("table")||support("indexes");$ek=h(ME)."select=".urlencode($Q);$ml=h(ME)."table=".urlencode($Q);if($this->settings->isSelectionPreferred()){if($this->settings->isNavigationReversed()&&$dl)echo" <a href='$ml' title='",lang(35),"' class='secondary'>",icon("structure"),"</a>";echo"<a href='$ek'",bold($wa,$yb)," data-primary='true' title='$A'>$A</a>";if($ad&&$dl)echo" <a href='$ml' title='",lang(35),"' class='secondary'>",icon_solo("structure"),"</a>";}else{if($this->settings->isNavigationReversed())echo" <a href='$ek' title='",lang(34),"' class='secondary'>",icon("data"),"</a>";if($dl)echo"<a href='$ml'",bold($wa,$yb)," data-primary='true' title='$A'>$A</a>";else
echo"<span data-primary='true'",bold($wa,$yb),">$A</span>";if($ad)echo" <a href='$ek' title='",lang(34),"' class='secondary'>",icon_solo("data"),"</a>";}echo"</li>\n";}echo"</menu></nav>\n",script("initTablesList(".json_encode($this->admin->getDatabase(),JSON_HEX_TAG).");");}function
getSettingsRows($Be){$O=parent::getSettingsRows($Be);if($Be==1){$C=[""=>lang(11),Config::$NavigationSimple=>lang(82),Config::$NavigationDual=>lang(83),Config::$NavigationHover=>lang(84),Config::$NavigationReversed=>lang(85)];$h=$C[$this->config->getNavigationMode()];$C[""].=" ($h)";$O["navigationMode"]="<tr><th>".lang(86)."</th>"."<td>".html_radios("navigationMode",$C,($ra=$this->settings->getParameter("navigationMode"))!==null?$ra:"")."<span class='input-hint'>".lang(87)."</span>"."</td></tr>\n";$C=[""=>lang(11),0=>lang(35),1=>lang(34),];$h=$C[$this->config->isSelectionPreferred()?1:0];$C[""].=" ($h)";$O["preferSelection"]="<tr><th id='label-links'>".lang(88)."</th>"."<td>".html_select("preferSelection",$C,($ra=$this->settings->getParameter("preferSelection"))!==null?$ra:"","","label-links",true)."<span class='input-hint'>".lang(89)."</span>"."</td></tr>\n";}return$O;}function
getForeignColumnInfo(array$ge,$b){return
null;}}class
TmpFile{private$handler;private$size;function
__construct(){$this->handler=tmpfile();}function
getSize(){return$this->size;}function
write($Yb){if(!$this->handler)return;$this->size+=strlen($Yb);fwrite($this->handler,$Yb);}function
send(){if(!$this->handler)return;fseek($this->handler,0);fpassthru($this->handler);fclose($this->handler);}}function
print_select_result(Result$I,$d=null,array$fi=[],$v=0){$rg=[];$s=[];$c=[];$ab=[];$fm=[];$J=[];for($p=0;(!$v||$p<$v)&&($K=$I->fetchRow());$p++){if(!$p){echo"<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>";for($If=0;$If<count($K);$If++){$j=$I->fetchField();if(!$j){echo"<th></th>";continue;}$A=$j->name;$ei=isset($j->orgtable)?$j->orgtable:"";$di=isset($j->orgname)?$j->orgname:$A;if(isset($j->table))$J[$j->table]=$ei;if($fi&&DIALECT=="sql")$rg[$If]=($A=="table"?"table=":($A=="possible_keys"?"indexes=":null));elseif($ei!=""){if(!isset($s[$ei])){$s[$ei]=[];foreach(indexes($ei,$d)as$r){if($r["type"]=="PRIMARY"){$s[$ei]=array_flip($r["columns"]);break;}}$c[$ei]=$s[$ei];}if(isset($c[$ei][$di])){unset($c[$ei][$di]);$s[$ei][$di]=$If;$rg[$If]=$ei;}}if($j->charsetnr==63)$ab[$If]=true;$fm[$If]=$j->type;$T=trim(($ei!=""?"$ei.$di":($j->name!=$di?$di:""))." ".Driver::get()->getTypeName($j));echo"<th".($T!=""?" title='".h($T)."'":"").">".h($A).($fi?doc_link(['sql'=>"explain-output.html#explain_".strtolower($A),'mariadb'=>"reference/sql-statements/administrative-sql-statements/analyze-and-explain-statements/explain#columns-in-explain-...-select",]):"");}echo"</thead>\n";}echo"<tr>";foreach($K
as$t=>$X){$w="";if(isset($rg[$t])&&!$c[$rg[$t]]){if($fi&&DIALECT=="sql"){$Q=$K[array_search("table=",$rg)];$w=ME.$rg[$t].urlencode($fi[$Q]!=""?$fi[$Q]:$Q);}else{$w=ME."edit=".urlencode($rg[$t]);foreach($s[$rg[$t]]as$_b=>$If)$w
.="&where".urlencode("[".bracket_escape($_b)."]")."=".urlencode($K[$If]);}}$U=($ab[$t]?'blob':($fm[$t]==254?'char':''));$j=['full_type'=>$U,'type'=>$U,];$X=select_value($X,$w,$j,null);$yb=$fm[$t]<=9||$fm[$t]==246?"class='number'":"";echo"<td $yb>$X</td>";}}if($p)echo"</table>\n</div>";else
echo"<p class='message'>".lang(90);echo"\n";return$J;}function
referencable_primary($jk){$J=[];foreach(table_status('',true)as$ol=>$Q){if($ol!=$jk&&fk_support($Q)){foreach(fields($ol)as$j){if($j["primary"]){if($J[$ol]){unset($J[$ol]);break;}$J[$ol]=$j;}}}}return$J;}function
textarea($A,$Y,$L=10,$Gb=80){echo"<textarea name='".h($A)."' rows='$L' cols='$Gb' class='sqlarea jush-".DIALECT."' spellcheck='false' wrap='off'>";if(is_array($Y)){foreach($Y
as$X)echo
h($X[0])."\n\n\n";}else
echo
h($Y);echo"</textarea>";}function
select_input($Ma,$C,$Y="",$Nh="",$Ki=""){if($C&&$Y!=""&&!isset($C[$Y]))$C=[$Y=>$Y]+$C;$yl=($C?"select":"input");return"<$yl $Ma".($C?"><option value=''>$Ki".optionlist($C,$Y,true)."</select>":" size='10' value='".h($Y)."' placeholder='$Ki'>").($Nh?script("qsl('$yl').onchange = $Nh;",""):"");}function
json_row($t,$X=null){static$Zd=true;if($Zd)echo"{";if($t!=""){echo($Zd?"":",")."\n\t\"".addcslashes($t,"\r\n\t\"\\/").'": '.($X!==null?'"'.addcslashes($X,"\r\n\t\"\\/").'"':'null');$Zd=false;}else{echo"\n}\n";$Zd=true;}}function
edit_type($t,$j,$Cb,$he=[],$Id=[]){$U=isset($j["type"])?$j["type"]:null;echo'<td><select name="',h($t),'[type]" class="type" aria-labelledby="label-type">';$Uc=Driver::get()->getTypes();if($U&&!isset($Uc[$U])&&!isset($he[$U])&&!in_array($U,$Id))$Id[]=$U;$Vk=Driver::get()->getStructuredTypes();if($he)$Vk[lang(91)]=$he;echo
optionlist(array_merge($Id,$Vk),$U),'</select><td><input name="',h($t),'[length]" value="',h(isset($j["length"])?$j["length"]:null),'" size="3"',(!(isset($j["length"])?$j["length"]:null)&&preg_match('~var(char|binary)$~',$U)?" class='input required'":" class='input'"),' aria-labelledby="label-length"><td class="options">',($Cb?"<select name='".h($t)."[collation]'".option_types($U,'(char|text|enum|set)$').'><option value="">('.lang(92).')'.optionlist($Cb,isset($j["collation"])?$j["collation"]:null).'</select>':''),(Driver::get()->getUnsigned()?"<select name='".h($t)."[unsigned]'".option_types($U,'^$|'.number_type()).'><option>'.optionlist(Driver::get()->getUnsigned(),isset($j["unsigned"])?$j["unsigned"]:null).'</select>':''),(isset($j['on_update'])?"<select name='".h($t)."[on_update]'".option_types($U,'timestamp|datetime').'>'.optionlist([""=>"(".lang(93).")","CURRENT_TIMESTAMP"],(preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?"CURRENT_TIMESTAMP":$j["on_update"])).'</select>':''),($he?"<select name='".h($t)."[on_delete]'".option_types($U,'`')."><option value=''>(".lang(94).")".optionlist(Driver::get()->getOnActions(),isset($j["on_delete"])?$j["on_delete"]:null)."</select> ":" ");}function
option_types($U,$fm){return" data-types='".h($fm)."'".(preg_match("~$fm~",$U)?"":" class='hidden'");}function
process_length($u){$qd=Driver::$EnumLengthPattern;return(preg_match("~^\\s*\\(?\\s*$qd(?:\\s*,\\s*$qd)*+\\s*\\)?\\s*\$~",$u)&&preg_match_all("~$qd~",$u,$z)?"(".implode(",",$z[0]).")":preg_replace('~^[0-9].*~','(\0)',preg_replace('~[^-0-9,+()[\]]~','',$u)));}function
process_type($j,$Ab="COLLATE"){return" $j[type]".process_length($j["length"]).(preg_match(number_type(),$j["type"])&&in_array($j["unsigned"],Driver::get()->getUnsigned())?" $j[unsigned]":"").(preg_match('~char|text|enum|set~',$j["type"])&&$j["collation"]?" $Ab ".(DIALECT=="mssql"?$j["collation"]:q($j["collation"])):"");}function
process_field($j,$dm){if($j["on_update"])$j["on_update"]=preg_replace('~current_timestamp(\(\))?~i',"CURRENT_TIMESTAMP",$j["on_update"]);return[idf_escape(trim($j["field"])),process_type($dm),($j["null"]?" NULL":" NOT NULL"),default_value($j),(preg_match('~timestamp|datetime~',$j["type"])&&$j["on_update"]?" ON UPDATE ".$j["on_update"]:""),(support("comment")&&$j["comment"]!=""?" COMMENT ".q(normalize_newlines($j["comment"])):""),($j["auto_increment"]?auto_increment():null),];}function
normalize_newlines($Y){return
str_replace("\r","",(string)$Y);}function
default_value($j){if($j["default"]===null)return"";$h=normalize_newlines($j["default"]);$te=$j["generated"];if(in_array($te,Driver::get()->getGenerated())){if(DIALECT=="mssql")return" AS ($h)".($te=="VIRTUAL"?"":" $te");else
return" GENERATED ALWAYS AS ($h) $te";}if(stripos($h,"GENERATED ")===0)return" $h";if(preg_match('~char|binary|text|json|enum|set~',$j["type"])||preg_match('~^(?![a-z])~i',$h)){if(DIALECT=="sql"&&preg_match('~text|json~',$j["type"]))return" DEFAULT (".q($h).")";else
return" DEFAULT ".q($h);}else{$h=str_ireplace("current_timestamp()","CURRENT_TIMESTAMP",$h);return" DEFAULT ".(DIALECT=="sqlite"?"($h)":$h);}}function
type_class($U){foreach(['char'=>'text','date'=>'time|year','binary'=>'blob','enum'=>'set',]as$yb=>$Hi){if(preg_match("~$yb|$Hi~",$U))return"class='$yb'";}return"";}function
edit_fields(array$k,array$Cb,$U="TABLE",$he=[]){$k=array_values($k);$Ob=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Mb=$Ob?"":"class='hidden'";echo"<thead><tr>\n";if(support("move_col"))echo"<th class='jsonly'></th>";if($U=="PROCEDURE")echo"<td></td>";echo"<th id='label-name'>",($U=="TABLE"?lang(95):lang(96)),"</th>\n","<td id='label-type'>",lang(45),"<textarea id='enum-edit' rows='4' cols='12' wrap='off' hidden></textarea>",script("gid('enum-edit').onblur = onFieldLengthBlur;"),"</td>\n","<td id='label-length'>",lang(97),"</td>\n","<td>",lang(98),"</td>\n";if($U=="TABLE")echo"<td id='label-null'>NULL</td>\n","<td><input type='radio' name='auto_increment_col' value=''><abbr id='label-ai' title='",lang(48),"'>AI</abbr>",doc_link(['sql'=>"example-auto-increment.html",'mariadb'=>"reference/data-types/auto_increment",]),"</td>\n","<td id='label-default'>",lang(49),"</td>\n",support("comment")?"<td id='label-comment' $Mb>".lang(47)."</td>\n":"";echo"<td>","<button name='add[",(support("move_col")?0:count($k)),"]' value='1' title='",lang(99),"' class='button light'>",icon_solo("add"),"</button>",(support("move_col")?"":script("qsl('button').onclick = onAddLastFieldRowClick;")),script("row_count = ".count($k).";"),"</td>\n","</tr></thead>\n";$yb=support("move_col")?"class='sortable'":"";echo"<tbody $yb>\n";foreach($k
as$p=>$j){$p++;$gi=$j[($_POST?"orig":"field")];$Kc=(isset($_POST["add"][$p-1])||(isset($j["field"])&&!(isset($_POST["drop_col"][$p])?$_POST["drop_col"][$p]:null)))&&(support("drop_col")||$gi=="");echo"<tr",($Kc?"":" hidden"),">\n";if(support("move_col"))echo"<th class='handle jsonly'>",icon_solo("handle"),"</td>";if($U=="PROCEDURE")echo"<td>",html_select("fields[$p][inout]",Driver::get()->getInOut(),$j["inout"]),"</td>\n";echo"<th>";if($Kc)echo"<input class='input' name='fields[$p][field]' value='",h($j["field"]),"' data-maxlength='64' autocapitalize='off' aria-labelledby='label-name' ".(isset($_POST["add"][$p-1])?"autofocus":"").">";echo
input_hidden("fields[$p][orig]",$gi);edit_type("fields[$p]",$j,$Cb,$he);echo"</th>\n";if($U=="TABLE"){echo"<td>",checkbox("fields[$p][null]",1,$j["null"],"","","block","label-null"),"</td>\n";$tb=$j["auto_increment"]?"checked":"";echo"<td><label class='block'><input type='radio' name='auto_increment_col' value='$p' $tb aria-labelledby='label-ai'></label></td>\n","<td class='default-value'>";if(Driver::get()->getGenerated())echo
html_select("fields[$p][generated]",array_merge(["","DEFAULT"],Driver::get()->getGenerated()),$j["generated"]);else
echo
checkbox("fields[$p][generated]",1,$j["generated"],"","","","label-default");$Ma="name='fields[$p][default]' aria-labelledby='label-default'";$Y=h($j["default"]);if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";if(support("comment")){$Fg=Connection::get()->isMinVersion("5.5")?1024:255;$Ma="name='fields[$p][comment]' data-maxlength='$Fg' aria-labelledby='label-comment'";$Y=h($j["comment"]);echo"<td $Mb>";if(str_contains($Y,"\n")){if($Y[0]=="\n")$Y="\n$Y";echo"<textarea $Ma rows='3' cols='30' style='vertical-align: bottom;'>$Y</textarea>";}else
echo"<input class='input' $Ma value='$Y'>";echo"</td>\n";}}echo"<td>";if(support("move_col"))echo"<button name='add[$p]' value='1' title='".lang(99)."' class='button light'>",icon_solo("add"),"</button>","<button name='up[$p]' value='1' title='".lang(100)."' class='button light hidden'>",icon_solo("arrow-up"),"</button>","<button name='down[$p]' value='1' title='".lang(101)."' class='button light hidden'>",icon_solo("arrow-down"),"</button>";if($gi==""||support("drop_col"))echo"<button name='drop_col[$p]' value='1' title='".lang(59)."' class='button light'>",icon_solo("remove"),"</button>";echo"</td>\n</tr>\n";}echo"</tbody>";}function
process_fields(&$k){$Ch=0;if($_POST["up"]){$bg=0;foreach($k
as$t=>$j){if(key($_POST["up"])==$t){unset($k[$t]);array_splice($k,$bg,0,[$j]);break;}if(isset($j["field"]))$bg=$Ch;$Ch++;}}elseif($_POST["down"]){$me=false;foreach($k
as$t=>$j){if(isset($j["field"])&&$me){unset($k[key($_POST["down"])]);array_splice($k,$Ch,0,[$me]);break;}if(key($_POST["down"])==$t)$me=$j;$Ch++;}}elseif($_POST["add"]){$k=array_values($k);array_splice($k,key($_POST["add"]),0,[[]]);}elseif(!$_POST["drop_col"])return
false;return
true;}function
normalize_enum($y){$X=$y[0];return"'".str_replace("'","''",addcslashes(stripcslashes(str_replace($X[0].$X[0],$X[0],substr($X,1,-1))),'\\'))."'";}function
grant($xe,array$ej,$c,$Lh,$xm){if(!$ej)return
true;if($ej==["ALL PRIVILEGES","GRANT OPTION"]){if($xe)return(bool)queries("GRANT ALL PRIVILEGES ON $Lh TO $xm WITH GRANT OPTION");else
return
queries("REVOKE ALL PRIVILEGES ON $Lh FROM $xm")&&queries("REVOKE GRANT OPTION ON $Lh FROM $xm");}if($ej==["GRANT OPTION","PROXY"]){if($xe)return(bool)queries("GRANT PROXY ON $Lh TO $xm WITH GRANT OPTION");else
return(bool)queries("REVOKE PROXY ON $Lh FROM $xm");}return(bool)queries(($xe?"GRANT ":"REVOKE ").preg_replace('~(GRANT OPTION)\([^)]*\)~','$1',implode("$c, ",$ej).$c)." ON $Lh ".($xe?"TO ":"FROM ").$xm);}function
drop_create($Wc,$ec,$Xc,$Cl,$Yc,$x,$Sg,$Qg,$Rg,$Jh,$th){if($_POST["drop"])query_redirect($Wc,$x,$Sg);elseif($Jh=="")query_redirect($ec,$x,$Rg);elseif($Jh!=$th){$hc=queries($ec);queries_redirect($x,$Qg,$hc&&queries($Wc));if($hc)queries($Xc);}else
queries_redirect($x,$Qg,queries($Cl)&&queries($Yc)&&queries($Wc)&&queries($ec));}function
create_trigger($Lh,array$Yl){$Ll=" $Yl[Timing] $Yl[Event]".(preg_match('~ OF~',$Yl["Event"])?" $Yl[Of]":"");return"CREATE TRIGGER ".idf_escape($Yl["Trigger"]).(DIALECT=="mssql"?$Lh.$Ll:$Ll.$Lh).rtrim(" $Yl[Type]\n$Yl[Statement]",";").";";}function
create_routine($Mj,$K){$wk=[];$k=(array)$K["fields"];ksort($k);$ff=implode("|",Driver::get()->getInOut());foreach($k
as$j){if($j["field"]!="")$wk[]=(preg_match("~^($ff)\$~",$j["inout"])?"$j[inout] ":"").idf_escape($j["field"]).process_type($j,"CHARACTER SET");}$zc=rtrim($K["definition"],";");return"CREATE $Mj ".idf_escape(trim($K["name"]))." (".implode(", ",$wk).")".($Mj=="FUNCTION"?" RETURNS".process_type($K["returns"],"CHARACTER SET"):"").($K["language"]?" LANGUAGE $K[language]":"").(DIALECT=="pgsql"?" AS ".q($zc):"\n$zc;");}function
remove_definer($H){return
preg_replace('~^([A-Z =]+) DEFINER=`'.preg_replace('~@(.*)~','`@`(%|\1)',logged_user()).'`~','\1',$H);}function
format_foreign_key($n){$Mh=implode("|",Driver::get()->getOnActions());$g=$n["db"];$yh=$n["ns"];return" FOREIGN KEY (".implode(", ",array_map('AdminNeo\idf_escape',$n["source"])).") REFERENCES ".($g!=""&&$g!=$_GET["db"]?idf_escape($g).".":"").($yh!=""&&$yh!=$_GET["ns"]?idf_escape($yh).".":"").idf_escape($n["table"])." (".implode(", ",array_map('AdminNeo\idf_escape',$n["target"])).")".(preg_match("~^($Mh)\$~",$n["on_delete"])?" ON DELETE $n[on_delete]":"").(preg_match("~^($Mh)\$~",$n["on_update"])?" ON UPDATE $n[on_update]":"").(isset($n["deferrable"])?" $n[deferrable]":"");}function
tar_file($m,TmpFile$Pl){$Le=pack("a100a8a8a8a12a12",$m,644,0,0,decoct($Pl->getSize()),decoct(time()));$vb=8*32;for($p=0;$p<strlen($Le);$p++)$vb+=ord($Le[$p]);$Le
.=sprintf("%06o",$vb)."\0 ";echo$Le,str_repeat("\0",512-strlen($Le));$Pl->send();echo
str_repeat("\0",511-($Pl->getSize()+511)%512);}function
doc_link(array$Gi,$Dl="<sup>?</sup>"){if(!(isset($Gi[DIALECT])?$Gi[DIALECT]:null))return"";$Fm=doc_version();$sm=['sql'=>"https://dev.mysql.com/doc/refman/$Fm/en/",'sqlite'=>"https://www.sqlite.org/",'pgsql'=>"https://www.postgresql.org/docs/".(Connection::get()->isCockroachDB()?"current":$Fm)."/",'mssql'=>"https://learn.microsoft.com/en-us/sql/",'oracle'=>"https://www.oracle.com/pls/topic/lookup?ctx=db".str_replace(".","",$Fm)."&id=",'elastic'=>"https://www.elastic.co/guide/en/elasticsearch/reference/$Fm/",];if(Connection::get()->isMariaDB()){$sm['sql']="https://mariadb.com/docs/server/";$Gi['sql']=isset($Gi['mariadb'])?$Gi['mariadb']:str_replace(".html","",$Gi['sql']);}return"<a href='".h($sm[DIALECT].$Gi[DIALECT].(DIALECT=='mssql'?"?view=sql-server-ver$Fm":""))."'".target_blank().">$Dl</a>";}function
doc_version(){return
preg_replace('~^(\d\.?\d).*~s','\1',Connection::get()->getVersion());}function
db_size($g){if(!Connection::get()->selectDatabase($g))return"?";$J=0;foreach(table_status()as$R)$J+=$R["Data_length"]+$R["Index_length"];return
format_number($J);}function
set_utf8mb4($ec){static$wk=false;if(!$wk&&preg_match('~\butf8mb4~i',$ec)){$wk=true;echo"SET NAMES ".charset(Connection::get()).";\n\n";}}error_reporting(E_ALL&~E_DEPRECATED);set_error_handler(function($sd,$i){return(bool)preg_match('~^Undefined (array key|offset|index)~',$i);},E_WARNING|E_NOTICE);;$Wd=!preg_match('~^(unsafe_raw)?$~',ini_get("filter.default"));if($Wd||ini_get("filter.default_flags")){foreach(['_GET','_POST','_COOKIE','_SERVER']as$X){$mm=filter_input_array(constant("INPUT$X"),FILTER_UNSAFE_RAW);if($mm)$$X=$mm;}}if(function_exists("mb_internal_encoding"))mb_internal_encoding("8bit");class
Server{private$params;private$key;function
__construct(array$ti,$t=null){$this->params=$ti;$this->key=$t;}function
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
getConfigParams(){$ti=isset($this->params["config"])?$this->params["config"]:[];$we=["servers"];foreach($we
as$si){if(isset($ti[$si]))unset($ti[$si]);}return$ti;}}class
Config{static$NavigationSimple="simple";static$NavigationDual="dual";static$NavigationHover="hover";static$NavigationReversed="reversed";private$params;private$servers=[];function
__construct(array$ti){$this->params=$ti;if(isset($this->params["servers"])){foreach($this->params["servers"]as$t=>$N){$ok=new
Server($N,is_string($t)?$t:null);$this->params["servers"][$t]=$ok;$this->servers[$ok->getKey()]=$ok;}}}function
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
getDefaultDriver(array$Vc){$Tc=isset($this->params["defaultDriver"])?$this->params["defaultDriver"]:null;return$Tc&&isset($Vc[$Tc])?$Tc:key($Vc);}function
getDefaultServer(){$N=isset($this->params["defaultServer"])?$this->params["defaultServer"]:null;if($N===null)return
null;$ok=isset($this->params["servers"][$N])?$this->params["servers"][$N]:null;if($ok)return$ok->getKey();return$N;}function
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
getServerPairs(array$Vc){$Bk=null;foreach($this->servers
as$N){if(!isset($Vc[$N->getDriver()]))continue;if(!$Bk)$Bk=$N->getDriver();elseif($N->getDriver()!=$Bk){$Bk=null;break;}}$pk=[];foreach($this->servers
as$t=>$N){if(!isset($Vc[$N->getDriver()]))continue;$nk=$N->getName();if($Bk&&$nk)$pk[$t]=$nk;else$pk[$t]=$Vc[$N->getDriver()].($nk!=""?" - $nk":"");}return$pk;}function
getServer($mk){return
isset($this->servers[$mk])?$this->servers[$mk]:null;}function
applyServer($N){$N=$this->getServer($N);if(!$N)return;$this->params=array_merge($this->params,$N->getConfigParams());}private
function
parseList($tg){if(is_array($tg))return$tg;return
preg_split('~\s*,\s*~',(string)$tg);}}class
Settings{private
static$CookieName="neo_settings";static$ColorSchemeLight="light";static$ColorSchemeDark="dark";static$NavigationWidthMin=10;static$NavigationWidthMax=30;private$config;private$params=[];function
__construct(Config$Tb){$this->config=$Tb;if(isset($_COOKIE[self::$CookieName])){parse_str($_COOKIE[self::$CookieName],$this->params);$this->save();}if(isset($_COOKIE["neo_lang"])){$this->updateParameter("lang",$_COOKIE["neo_lang"]);unset($_COOKIE["neo_lang"]);cookie("neo_lang","",-3600);}}static
function
readParameter($t){parse_str(isset($_COOKIE[self::$CookieName])?$_COOKIE[self::$CookieName]:"",$ti);return
isset($ti[$t])?$ti[$t]:null;}function
getParameter($t,$h=null){return
isset($this->params[$t])?$this->params[$t]:$h;}function
updateParameter($t,$Y){$this->updateParameters([$t=>$Y]);}function
updateParameters(array$ti){$this->params=array_filter(array_merge($this->params,$ti),function($Y){return$Y!==null;});$this->save();}private
function
save(){cookie(self::$CookieName,http_build_query($this->params),7776000);}function
getTheme(){return($ra=$this->getParameter("theme"))!==null?$ra:$this->config->getTheme();}function
getColorScheme(){return$this->getParameter("colorScheme");}function
getNavigationMode(){return($ra=$this->getParameter("navigationMode"))!==null?$ra:$this->config->getNavigationMode();}function
isNavigationSimple(){return$this->getNavigationMode()==Config::$NavigationSimple;}function
isNavigationDual(){return$this->getNavigationMode()==Config::$NavigationDual;}function
isNavigationHover(){return$this->getNavigationMode()==Config::$NavigationHover;}function
isNavigationReversed(){return$this->getNavigationMode()==Config::$NavigationReversed;}function
getNavigationWidth(){$Wm=$this->getParameter("navigationWidth");if($Wm===null)return
null;return
min(max((float)$Wm,self::$NavigationWidthMin),self::$NavigationWidthMax);}function
isSelectionPreferred(){return($ra=$this->getParameter("preferSelection"))!==null?$ra:$this->config->isSelectionPreferred();}function
isRelationLinks(){return
isset($this->params["relationLinks"])?$this->params["relationLinks"]:$this->config->isRelationLinks();}function
getRecordsPerPage(){return($ra=$this->getParameter("recordsPerPage"))!==null?$ra:$this->config->getRecordsPerPage();}function
getEnumAsSelectThreshold(){$Y=$this->getParameter("enumAsSelectThreshold");if($Y<0)return
null;return$Y!==null?(int)$Y:$this->config->getEnumAsSelectThreshold();}}class
Hash{static
function
hkdf($u,$t,$kf="",$Rj=""){if(extension_loaded("hash")&&PHP_VERSION_ID>=70120)return
hash_hkdf("sha1",$t,$u,$kf,$Rj);if($Rj=="")$Rj=str_repeat("\0",20);$fj=self::hmacSha1($t,$Rj);$Gh="";for($Pf="",$bb=1;!isset($Gh[$u-1]);$bb++){$Pf=self::hmacSha1($Pf.$kf.chr($bb),$fj);$Gh
.=$Pf;}return
substr($Gh,0,$u);}static
function
hmacSha1($e,$t){if(!extension_loaded("hash"))return
hash_hmac("sha1",$e,$t,true);if(strlen($t)>64)$t=sha1($t,true);$t=str_pad($t,64,"\0");$zf=($t^str_repeat("\x36",64));$Qh=($t^str_repeat("\x5C",64));return
sha1($Qh.sha1($zf.$e,true),true);}}class
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
\Sodium\randombytes_buf($u);$lm=DIRECTORY_SEPARATOR==="/";if($lm){$I=self::readDevUrandom($u);if($I!==false)return$I;}$fb=$lm&&PHP_VERSION_ID>50609&&PHP_VERSION_ID<50613;if(extension_loaded("mcrypt")&&!$fb){$I=mcrypt_create_iv($u,MCRYPT_DEV_URANDOM);if($I!==false)return$I;}$gb=PHP_VERSION_ID<50444||(PHP_VERSION_ID>50500&&PHP_VERSION_ID<50528)||(PHP_VERSION_ID>50600&&PHP_VERSION_ID<50612);if(extension_loaded("openssl")&&!$gb){$I=openssl_random_pseudo_bytes($u,$Uk);if($Uk)return$I;}return
false;}private
static
function
readDevUrandom($u){static$l=null;if($l===null)$l=@fopen("/dev/urandom","rb");if(!$l)return
false;$Bj=$u;$I="";do{$e=fread($l,$Bj);if($e===false)return
false;$Bj-=strlen($e);$I
.=$e;}while($Bj>0);return$I;}private
static
function
readCapicom($u){$Ib=new
\COM("CAPICOM.Utilities.1");$Bj=$u;$I="";do{$e=base64_decode((string)$Ib->GetRandom($u,0));$Bj-=strlen($e);$I
.=$e;}while($Bj>0);return$I;}private
static
function
lastResortRandom($u){static$t=null;static$Rj=null;if($t===null){$e=$_SERVER;$e[]=uniqid("",true);shuffle($e);$t=sha1(serialize($e),true);if(extension_loaded("openssl"))$Rj=openssl_random_pseudo_bytes(20);else{$Rj="";for($p=0;$p<20;$p++)$Rj
.=chr((mt_rand()^mt_rand())%256);}}else{if((ord($t)%2===0)===(ord($Rj)%2===0))$t=Hash::hmacSha1($t,$Rj);else$Rj=Hash::hmacSha1($Rj,$t);}return
Hash::hkdf($u,$t,"$u",$Rj);}}if(!function_exists("str_starts_with")){function
str_starts_with($Ke,$ph){return
strpos($Ke,$ph)===0;}}if(!function_exists("str_contains")){function
str_contains($Ke,$ph){return
strpos($Ke,$ph)!==false;}}if(!function_exists("password_verify")){function
password_verify($F,$Je){return
false;}}if(!function_exists("ini_set")){function
ini_set($Wh,$Y){return
false;}}function
version(){return
VERSION;}function
idf_unescape($bf){if(!preg_match('~^[`\'"[]~',$bf))return$bf;$bg=substr($bf,-1);return
str_replace($bg.$bg,$bg,substr($bf,1,-1));}function
q($Tk){return
Connection::get()->quote($Tk);}function
number($X){return
preg_replace('~[^0-9]+~','',$X);}function
number_type(){return'((^|[^o])int(?!er)|numeric|real|float|double|decimal|money)';}function
remove_slashes(array$Cm,$Wd=false){$J=[];foreach($Cm
as$t=>$X)$J[stripslashes($t)]=(is_array($X)?remove_slashes($X,$Wd):($Wd?$X:stripslashes($X)));return$J;}function
bracket_escape($bf,$Ta=false){static$Vl=[':'=>':1',']'=>':2','['=>':3','"'=>':4'];return
strtr($bf,($Ta?array_flip($Vl):$Vl));}function
min_version($Fm,$Ag=null,$d=null){if(!$d)$d=Connection::get();if($Ag&&$d->isMariaDB())$Fm=$Ag;return$Fm&&$d->isMinVersion($Fm);}function
charset(Connection$d){return($d->isMinVersion("5.5.3")?"utf8mb4":"utf8");}function
link_files($A,array$Vd){switch($A){case'favicon-blue.ico':$m='favicon-blue-0f5ce53a66b1e25395d0048da369f19e__6bb95962.ico';break;case'favicon-blue.svg':$m='favicon-blue-17e440832c1eac07527560a0d6f0d2ee__6bb95962.svg';break;case'apple-touch-icon-blue.png':$m='apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__6bb95962.png';break;case'logo.svg':$m='logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg';break;case'jush.css':$m='jush-b3a93b18444da26820ff61746521dede__d6435f99.css';break;case'jush-dark.css':$m='jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css';break;case'jush.js':$m='jush-615bc0b9720a1de8edd2c6876a3495b6__42cd49bd.js';break;case'icons.svg':$m='icons-70163a2695280bf75edba563e7b5471b__2ec7793c.svg';break;case'default-blue.css':$m='default-blue-e7acfdb81453b86f081569afa115859e__9486a148.css';break;case'default-blue-dark.css':$m='default-blue-dark-1061ad7d216f143e3626560b92b66061__747ec25d.css';break;case'main.js':$m='main-0864f21d8576870afa2ea1b4d2bb6be8__bb1d1ac2.js';break;default:$m=null;break;}if(!$m)return
null;return
BASE_URL."?file=".urldecode($m);}function
ini_bool($Wh){$X=ini_get($Wh);return
preg_match('~^(on|true|yes)$~i',$X)||(int)$X;}function
ini_bytes($mf){$X=ini_get($mf);switch(strtolower(substr($X,-1))){case'g':$X=(int)$X*1024;case'm':$X=(int)$X*1024;case'k':$X=(int)$X*1024;}return$X;}function
max_input_vars($K,$mi){$Cg=(int)ini_get("max_input_vars");return($Cg?(int)floor(($Cg-$mi)/$K):0);}function
max_input_vars_error(){$mf="max_input_vars";return
lang(102,"$mf = ".(int)ini_get($mf));}function
sid(){static$J;if($J===null)$J=(session_id()&&!($_COOKIE&&ini_bool("session.use_cookies")));return$J;}function
save_driver_name($Tc,$N,$A){restart_session();$_SESSION["drivers"][$Tc][$N]=$A;stop_session();}function
get_driver_name($Tc,$N=null){return
isset($_SESSION["drivers"][$Tc][$N])?$_SESSION["drivers"][$Tc][$N]:Drivers::get($Tc);}function
save_login($Tc,$N,$V,$F,$g=""){$t=isset($_COOKIE["neo_key"])?$_COOKIE["neo_key"]:null;$_SESSION["pwds"][$Tc][$N][$V]=$t?[encrypt_string($F,$t)]:$F;$_SESSION["db"][$Tc][$N][$V][$g]=true;}function
delete_login($Tc,$N,$V){unset($_SESSION["pwds"][$Tc][$N][$V]);unset($_SESSION["db"][$Tc][$N][$V]);}function
get_password(){$F=get_session("pwds");if(is_array($F))return$_COOKIE["neo_key"]?decrypt_string($F[0],$_COOKIE["neo_key"]):false;return$F;}function
get_vals($H,$b=0){$J=[];$I=Connection::get()->query($H);if(is_object($I)){while($K=$I->fetchRow())$J[]=$K[$b];}return$J;}function
get_key_vals($H,$d=null,$xk=true){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchRow()){if($xk)$J[$K[0]]=$K[1];else$J[]=$K[0];}}return$J;}function
get_rows($H,$d=null,$i="<p class='error'>"){if(!$d)$d=Connection::get();$J=[];$I=$d->query($H);if(is_object($I)){while($K=$I->fetchAssoc())$J[]=$K;}elseif(!$I&&!is_object($d)&&$i&&(defined("AdminNeo\PAGE_HEADER")||$i=="-- "))echo$i.error()."\n";return$J;}function
unique_array(array$K,array$s){foreach($s
as$r){if(!preg_match("~PRIMARY|UNIQUE~",$r["type"])&&!$r["partial"])continue;$im=[];foreach($r["columns"]as$t){if(!isset($K[$t]))continue
2;$im[$t]=$K[$t];}return$im;}return
null;}function
escape_key($t){if(preg_match('(^([\w(]+)('.str_replace("_",".*",preg_quote(idf_escape("_"))).')([ \w)]+)$)',$t,$y))return$y[1].idf_escape(idf_unescape($y[2])).$y[3];return
idf_escape($t);}function
where($Z,$k=[]){$Sb=[];foreach((array)$Z["where"]as$t=>$X){$t=bracket_escape($t,true);$b=escape_key($t);$j=isset($k[$t])?$k[$t]:null;$Rd=isset($j["type"])?$j["type"]:null;$qe=isset($j["full_type"])?$j["full_type"]:null;$Af=$j&&(is_blob($j)||preg_match('~binary~',$Rd));if($Af&&!is_utf8($X))$Sb[]="$b = ".Driver::get()->quoteBinary($X);elseif(DIALECT=="sql"&&$Rd=="json")$Sb[]="$b = CAST(".q($X)." AS JSON)";elseif(DIALECT=="pgsql"&&preg_match('~^jsonb?$~',$qe))$Sb[]="$b::jsonb = ".q($X)."::jsonb";elseif(DIALECT=="sql"&&is_numeric($X)&&strpos($X,".")!==false)$Sb[]="$b LIKE ".q($X);elseif(DIALECT=="mssql"&&strpos($Rd,"datetime")===false)$Sb[]="$b LIKE ".q(preg_replace('~[_%[]~','[\0]',$X));else$Sb[]="$b = ".(isset($k[$t])?unconvert_field($k[$t],q($X)):q($X));if(DIALECT=="sql"&&preg_match('~char|text~',$Rd)&&preg_match("~[^ -@]~",$X))$Sb[]="$b = ".q($X)." COLLATE ".charset(Connection::get())."_bin";}foreach((array)$Z["null"]as$t)$Sb[]=escape_key($t)." IS NULL";return
implode(" AND ",$Sb);}function
where_columns($Z,$k=[]){$c=[];foreach((array)$Z["null"]as$t)$c[$t]=true;foreach((array)$Z["where"]as$t=>$X){$t=bracket_escape($t,true);foreach($k
as$A=>$j){if($t==$A||strpos($t,idf_escape($A))!==false)$c[$A]=true;}}return$c;}function
where_check($X,$k=[]){parse_str($X,$qb);remove_slashes([&$qb]);return
where($qb,$k);}function
where_link($p,$b,$Y,$Th="="){return"&where%5B$p%5D%5Bcol%5D=".urlencode($b)."&where%5B$p%5D%5Bop%5D=".urlencode(($Y!==null?$Th:"IS NULL"))."&where%5B$p%5D%5Bval%5D=".urlencode($Y);}function
convert_fields(array$c,array$k,array$M=[]){$I="";foreach($c
as$t=>$X){if($M&&!in_array(idf_escape($t),$M))continue;$La=convert_field($k[$t]);if($La)$I
.=", $La AS ".idf_escape($t);}return$I;}function
cookie_path(){return
strtr(preg_replace('~\?.*~','',$_SERVER["REQUEST_URI"]),[";"=>"%3B",","=>"%2C"]);}function
cookie($A,$Y,$lg=2592000){header("Set-Cookie: $A=".rawurlencode($Y).($lg?"; expires=".gmdate("D, d M Y H:i:s",time()+$lg)." GMT":"")."; path=".cookie_path().(HTTPS?"; secure":"")."; HttpOnly; SameSite=lax",false);}function
get_url($rm,$Zb){$J=@file_get_contents($rm,false,$Zb);if(function_exists('http_get_last_response_headers'))$http_response_header=($ra=http_get_last_response_headers())!==null?$ra:[];return[$J,isset($http_response_header)?$http_response_header:[]];}function
get_settings($cc="neo_settings"){parse_str(isset($_COOKIE[$cc])?$_COOKIE[$cc]:"",$O);return$O;}function
get_setting($t,$cc="neo_settings"){$O=get_settings($cc);return
isset($O[$t])?$O[$t]:null;}function
save_settings(array$O,$cc="neo_settings"){cookie($cc,http_build_query($O+get_settings($cc)));}function
restart_session(){if(!ini_bool("session.use_cookies")&&session_status()==PHP_SESSION_NONE)session_start();}function
stop_session($ee=false){$vm=ini_bool("session.use_cookies");if(!$vm||$ee){session_write_close();if($vm&&ini_set("session.use_cookies","0")===false)session_start();}}function&get_session($t){return$_SESSION[$t][DRIVER][SERVER][$_GET["username"]];}function
set_session($t,$X){$_SESSION[$t][DRIVER][SERVER][$_GET["username"]]=$X;}function
auth_url($Em,$N,$V,$g=null){$qm=remove_from_uri(implode("|",array_keys(Drivers::getList()))."|username|ext|".($g!==null?"db|":"").($Em=='mssql'||$Em=='pgsql'?"":"ns|").session_name());preg_match('~([^?]*)\??(.*)~',$qm,$y);return"$y[1]?".(sid()?session_name()."=".urlencode(session_id())."&":"").urlencode($Em)."=".urlencode($N)."&".($_GET["ext"]?"ext=".urlencode($_GET["ext"])."&":"")."username=".urlencode($V).($g!=""?"&db=".urlencode($g):"").($y[2]?"&$y[2]":"");}function
is_ajax(){return($_SERVER["HTTP_X_REQUESTED_WITH"]=="XMLHttpRequest");}function
redirect($x,$_=null){if($_!==null){restart_session();$_SESSION["messages"][preg_replace('~^[^?]*~','',($x!==null?$x:$_SERVER["REQUEST_URI"]))][]=$_;}if($x!==null){if($x=="")$x=".";header("Location: $x");exit;}}function
query_redirect($H,$x,$_,$tj=true,$zd=true,$Jd=false,$Jl=""){if($zd){$Pk=microtime(true);$Jd=!Connection::get()->query($H);$Jl=format_time($Pk);}$Kk=$H?Admin::get()->formatMessageQuery($H,$Jl,$Jd):"";if($Jd){Admin::get()->addError(error().$Kk.script("initToggles();"));return
false;}if($tj)redirect($x,$_.$Kk);return
true;}function
queries_redirect($x,$_,$tj){$kj=implode("\n",Queries::$queries);$Jl=format_time(Queries::$start);return
query_redirect($kj,$x,$_,$tj,false,!$tj,$Jl);}class
Queries{static$queries=[];static$start=0.0;}function
queries($H){if(!Queries::$start)Queries::$start=microtime(true);if(support("sql")){Queries::$queries[]=(preg_match('~;$~',$H)?"DELIMITER ;;\n$H;\nDELIMITER ":$H).";";return
Connection::get()->query($H);}else{Queries::$queries[]=$H;return[];}}function
apply_queries($H,array$S,$ud='AdminNeo\table'){foreach($S
as$Q){if(!queries("$H ".$ud($Q)))return
false;}return
true;}function
format_time($Pk){return
lang(103,max(0,microtime(true)-$Pk));}function
relative_uri(){return
str_replace(":","%3a",preg_replace('~^[^?]*/([^?]*)~','\1',$_SERVER["REQUEST_URI"]));}function
remove_from_uri($si=""){return
substr(preg_replace("~(?<=[?&])($si".(sid()?"":"|".session_name()).")=[^&]*&~",'',relative_uri()."&"),0,-1);}function
get_file($t,$uc=false,$Ac=""){$l=$_FILES[$t];if(!$l)return
null;foreach($l
as$t=>$X)$l[$t]=(array)$X;$J='';foreach($l["error"]as$t=>$i){if($i)return$i;$A=$l["name"][$t];$Ql=$l["tmp_name"][$t];$Xb=file_get_contents($uc&&preg_match('~\.gz$~',$A)?"compress.zlib://$Ql":$Ql);if($uc){$Pk=substr($Xb,0,3);if(function_exists("iconv")&&preg_match("~^\xFE\xFF|^\xFF\xFE~",$Pk))$Xb=iconv("utf-16","utf-8",$Xb);elseif($Pk=="\xEF\xBB\xBF")$Xb=substr($Xb,3);}if($Ac){if(!preg_match("~$Ac\\s*\$~",$Xb))$Xb
.=";";$Xb
.="\n\n";}$J
.=$Xb;}return$J;}function
upload_error($i){$Kg=($i==UPLOAD_ERR_INI_SIZE?ini_get("upload_max_filesize"):0);return($i?lang(104).($Kg?" ".lang(105,$Kg):""):lang(106));}function
repeat_pattern($Hi,$u){return
str_repeat("$Hi{0,65535}",$u/65535)."$Hi{0,".($u%65535)."}";}function
is_utf8($X){return(preg_match('~~u',$X)&&!preg_match('~[\0-\x8\xB\xC\xE-\x1F]~',$X));}function
format_number($X){return
strtr(number_format($X,0,".",lang(107)),preg_split('~~u',lang(108),-1,PREG_SPLIT_NO_EMPTY));}function
format_rows(array$R){$L=$R["Rows"];$Ia=($L&&(DIALECT=="sqlite"||(isset($R["Engine"])?$R["Engine"]:"")==(DIALECT=="pgsql"?"table":"InnoDB")));return($Ia?"~ ":"").format_number($L);}function
friendly_url($X){return
preg_replace('~\W~i','-',$X);}function
table_status1($Q,$Ld=false){$J=table_status($Q,$Ld);return($J?reset($J):["Name"=>$Q]);}function
column_foreign_keys($Q){$J=[];foreach(Admin::get()->getForeignKeys($Q)as$n){foreach($n["source"]as$X)$J[$X][]=$n;}return$J;}function
fields_from_edit(){$J=[];foreach((array)$_POST["field_keys"]as$t=>$X){if($X!=""){$X=bracket_escape($X);$_POST["function"][$X]=$_POST["field_funs"][$t];$_POST["fields"][$X]=$_POST["field_vals"][$t];}}foreach((array)$_POST["fields"]as$t=>$X){$A=bracket_escape($t,true);$J[$A]=["field"=>$A,"full_type"=>"varchar","type"=>"varchar","privileges"=>["insert"=>1,"update"=>1,"where"=>1,"order"=>1],"null"=>true,"auto_increment"=>($t==Driver::get()->primary),];}return$J;}function
dump_headers($Ze,$gh=false){$Ze=friendly_url($Ze).date("-Ymd-His");$Fd=Admin::get()->sendDumpHeaders($Ze,$gh);$oi=$_POST["output"];if($oi!="text")header("Content-Disposition: attachment; filename=$Ze.$Fd".($oi!="file"&&preg_match('~^[0-9a-z]+$~',$oi)?".$oi":""));session_write_close();if(!ob_get_level())ob_start(null,4096);ob_flush();flush();return$Fd;}function
dump_table_order(array$nh,array$zj){$Uf=array_flip($nh);$bi=[];$Nm=[];$mc=false;$Mm=function($A)use(&$Mm,&$bi,&$Nm,&$mc,$Uf,$zj){if(isset($bi[$A]))return;if(isset($Nm[$A])){$mc=true;return;}$Nm[$A]=true;foreach(isset($zj[$A])?$zj[$A]:[]as$xj){if(isset($Uf[$xj]))$Mm($xj);}unset($Nm[$A]);$bi[$A]=true;};foreach($nh
as$A)$Mm($A);return($mc?null:array_keys($bi));}function
dump_csv($K){$cm=$_POST["format"]=="tsv";foreach($K
as$t=>$X){if(preg_match('~["\n]|^0[^.]|\.\d*0$|'.($cm?'\t':'[,;]|^$').'~',$X))$K[$t]='"'.str_replace('"','""',$X).'"';}echo
implode(($_POST["format"]=="csv"?",":($cm?"\t":";")),$K)."\r\n";}function
apply_sql_function($o,$b){return($o?($o=="unixepoch"?"DATETIME($b, '$o')":($o=="count distinct"?"COUNT(DISTINCT ":strtoupper("$o("))."$b)"):$b);}function
get_temp_dir(){$Fi=ini_get("upload_tmp_dir");if(!$Fi)$Fi=sys_get_temp_dir();return$Fi;}function
open_file_with_lock($m){if(is_link($m))return
null;$l=@fopen($m,"c+");if(!$l)return
null;@chmod($m,0660);if(!flock($l,LOCK_EX)){fclose($l);return
null;}return$l;}function
write_and_unlock_file($l,$e){rewind($l);fwrite($l,$e);ftruncate($l,strlen($e));unlock_file($l);}function
unlock_file($l){flock($l,LOCK_UN);fclose($l);}function
first(array$Ka){return
reset($Ka);}function
get_private_key($ec){$m=get_temp_dir()."/adminneo.key";if(!$ec&&!file_exists($m))return
false;$l=open_file_with_lock($m);if(!$l)return
false;$t=stream_get_contents($l);if(!$t){$t=Random::strongKey();write_and_unlock_file($l,$t);}else
unlock_file($l);return$t;}function
get_random_string(){return
Random::strongKey();}function
select_value($X,$w,$j,$Fl){if(is_array($X)){$J="";if(array_filter($X,'is_array')==array_values($X)){$Qf=[];foreach($X
as$W)$Qf+=array_fill_keys(array_keys($W),null);foreach(array_keys($Qf)as$Lf)$J
.="<th>".h($Lf);foreach($X
as$W){$J
.="<tr>";foreach(array_merge($Qf,$W)as$_m)$J
.="<td>".select_value($_m,$w,$j,$Fl);}}else{foreach($X
as$Lf=>$W)$J
.="<tr>".($X!=array_values($X)?"<th>".h($Lf):"")."<td>".select_value($W,$w,$j,$Fl);}return"<table>$J</table>";}$Wj="";if($j&&$X!==null&&($Fl===null||strlen($X)<=$Fl)&&($Cm=Driver::get()->explodeArrayValue($X,$j["full_type"],$Wj))){$Vj=$j;$Vj["type"]=$Vj["full_type"]=$Wj;$J=select_array_value($Cm,$X,$w,$Vj,$Fl);return
Driver::get()->implodeArrayValues($J,$j["full_type"]);}if(!$w)$w=Admin::get()->getFieldValueLink($X,$j);if($j)$X=Connection::get()->formatValue($X,$j);$J=$j?Admin::get()->formatFieldValue($X,$j):$X;if($J!==null){if(!is_utf8($J))$J="\0";elseif($Fl!=""&&is_shortable($j))$J=truncate_utf8($J,max(0,+$Fl));else$J=h($J);}return
Admin::get()->formatSelectionValue($J,$w,$j,$X);}function
select_array_value(array$Cm,$X,$w,array$j,$Fl){$I=[];foreach($Cm
as$Y){if(is_array($Y))$I[]=select_array_value($Y,$X,$w,$j,$Fl);else{$Vf=preg_replace('~(where%5B\d+%5D%5Bval%5D=)'.preg_quote(urlencode($X),"~")."~",'${1}'.urlencode($Y),$w);$I[]=select_value($Y,$Vf,$j,$Fl);}}return$I;}function
is_blob(array$j){$fm=Driver::get()->getStructuredTypes();$U=lang(109);return
preg_match('~blob|bytea|raw|file'.(DIALECT=="mssql"?'|binary|image':'').'~',$j["type"])&&!in_array($j["type"],isset($fm[$U])?$fm[$U]:[]);}function
is_generated_always(array$j){return(isset($j["generated"])?$j["generated"]:"")!=""||stripos((string)(isset($j["default"])?$j["default"]:""),"GENERATED ALWAYS AS ")===0;}function
is_mail($Y){return
is_string($Y)&&filter_var($Y,FILTER_VALIDATE_EMAIL);}function
is_web_url($Y){if(!is_string($Y)||!preg_match('~^(https?:)?//~i',$Y))return
false;$Pb=parse_url($Y);if(!$Pb)return
false;$rm=$Y;if(isset($Pb['path'])){$md=array_map('urlencode',explode('/',$Pb['path']));$rm=str_replace($Pb['path'],implode('/',$md),$rm);}if(isset($Pb['query'])){parse_str($Pb['query'],$ti);$rm=str_replace($Pb['query'],http_build_query($ti),$rm);}if(!isset($Pb['scheme']))$rm="https:$rm";return(bool)filter_var($rm,FILTER_VALIDATE_URL);}function
is_shortable($j){return$j&&!preg_match('~'.number_type().'|date|time|year~',$j["type"]);}function
host_port($N){return(preg_match('~^(:([^:].*)|(\[(.+)]|(([^:]+://)?[^:]+))(:(\d+))?)$~',$N,$y)?[(isset($y[4])?$y[4]:"").(isset($y[5])?$y[5]:""),$y[2].(isset($y[8])?$y[8]:"")]:[$N,'']);}function
count_rows($Q,$Z,$Bf,$_e){$H=" FROM ".table($Q).($Z?" WHERE ".implode(" AND ",$Z):"");return($Bf&&(DIALECT=="sql"||count($_e)==1)?"SELECT COUNT(DISTINCT ".implode(", ",$_e).")$H":"SELECT COUNT(*)".($Bf?" FROM (SELECT 1$H GROUP BY ".implode(", ",$_e).") x":$H));}function
slow_query($H){$g=Admin::get()->getDatabase();$Kl=Admin::get()->getQueryTimeout();$Dk=Driver::get()->slowQuery($H,$Kl);$d=null;if(!$Dk&&support("kill")){$d=connect();if($d&&($g==""||$d->selectDatabase($g))){$Sf=number($d->getValue(connection_id()));echo'<script',nonce(),'>
	const timeout = setTimeout(() => {
		ajax(\'',js_escape(ME),'script=kill\', function() {
		}, \'kill=',$Sf,'&token=',get_token(),'\');
	}, ',1000*$Kl,');
</script>
';}}ob_flush();flush();$J=@get_key_vals(($Dk?:$H),$d,false);if($d){echo
script("clearTimeout(timeout);");ob_flush();flush();}return$J;}function
get_token(){$pj=rand(1,1e6);return($pj^$_SESSION["token"]).":$pj";}function
verify_token(){list($Rl,$pj)=explode(":",$_POST["token"]);return($pj^$_SESSION["token"])==$Rl&&in_array($_SERVER["HTTP_SEC_FETCH_SITE"],["","same-origin"]);}function
script($Hk,$Ul="\n"){return"<script".nonce().">$Hk</script>$Ul";}function
script_src($rm,$yc=false){return"<script src='".h($rm)."'".nonce().($yc?" defer":"")."></script>\n";}function
nonce(){return' nonce="'.get_nonce().'"';}function
input_hidden($A,$Y=""){return"<input type='hidden' name='".h($A)."' value='".h($Y)."'>";}function
input_token(){return
input_hidden("token",get_token());}function
target_blank(){return' target="_blank" rel="noreferrer noopener"';}function
h($Tk){if($Tk===null||$Tk==="")return"";return
str_replace(["&","<","\"","'","\0"],["&amp;","&lt;","&quot;","&#039;","&#0;"],$Tk);}function
truncate_utf8($Tk,$u=80){if($Tk=="")return"";if(!preg_match("(^(".repeat_pattern("[\t\r\n -\x{10FFFF}]",$u).")($)?)u",$Tk,$y))preg_match("(^(".repeat_pattern("[\t\r\n -~]",$u).")($)?)",$Tk,$y);return
h($y[1]).(isset($y[2])?"":"<i>…</i>");}function
icon_solo($q){return
icon($q,"solo");}function
icon_chevron_down(){return
icon("chevron-down","chevron");}function
icon_chevron_right(){return
icon("chevron-down","chevron-right");}function
icon($q,$yb=null){$q=h($q);return"<svg class='icon ic-$q $yb'><use href='".link_files("icons.svg",[])."#$q'/></svg>";}function
checkbox($A,$Y,$tb,$Wf="",$Oh="",$yb="",$Yf=""){$J="<input type='checkbox' name='$A' value='".h($Y)."'".($tb?" checked":"").($Yf?" aria-labelledby='$Yf'":"").">".($Oh?script("qsl('input').onclick = function () { $Oh };",""):"");return($Wf!=""||$yb?"<label".($yb?" class='$yb'":"").">$J".h($Wf)."</label>":$J);}function
optionlist($C,$gk=null,$wm=false){$J="";foreach($C
as$Lf=>$W){$Yh=[$Lf=>$W];if(is_array($W)){$J
.='<optgroup label="'.h($Lf).'">';$Yh=$W;}foreach($Yh
as$t=>$X)$J
.='<option'.($wm||is_string($t)?' value="'.h($t).'"':'').($gk!==null&&($wm||is_string($t)?(string)$t:$X)===$gk?' selected':'').'>'.h($X);if(is_array($W))$J
.='</optgroup>';}return$J;}function
html_select($A,$C,$Y="",$Nh="",$Yf="",$wm=false){static$Wf=0;$Xf="";if(!$Yf&&substr(isset($C[""])?$C[""]:"",0,1)=="("){$Wf++;$Yf="label-$Wf";$Xf="<option value='' id='$Yf'>".h($C[""]);unset($C[""]);}return"<select name='".h($A)."'".($Yf?" aria-labelledby='$Yf'":"").">".$Xf.optionlist($C,$Y,$wm)."</select>".($Nh?script("qsl('select').onchange = function () { $Nh };",""):"");}function
html_radios($A,$C,$Y=""){$I="<span class='labels'>";foreach($C
as$t=>$X)$I
.="<label><input type='radio' name='".h($A)."' value='".h($t)."'".($t==$Y?" checked":"").">".h($X)."</label>";$I
.="</span>";return$I;}function
confirm($_="",$ik="qsl('input')"){return
script("$ik.onclick = () => confirm('".js_escape($_?:lang(110))."');","");}function
print_fieldset_start($q,$hg,$Ye,$Km=false,$Fk=false){echo"<fieldset id='fieldset-$q' class='closable ".(!$Km?" closed":"")."'>","<legend><a href='#'>$hg</a></legend>",icon($Ye,"fieldset-icon jsonly"),"<div class='fieldset-content".($Fk?" sortable":"")."'>";}function
print_fieldset_end($q,$Fk=false){echo"</div>",script("initFieldset('$q');","");if($Fk)echo
script("initSortable('#fieldset-$q .fieldset-content');","");echo"</fieldset>\n";}function
bold($cb,$yb=""){return($cb?" class='$yb active'":($yb?" class='$yb'":""));}function
js_escape($Tk){return
str_replace("<","\\x3C",addcslashes($Tk,"\r\n'\\"));}function
js_escape_key($Tk){return'"'.str_replace("<","\\x3C",addcslashes($Tk,"\r\n\t\"\\")).'"';}function
js_escape_re($Tk){return
addcslashes(preg_quote($Tk,"/"),"\r\n");}function
pagination($E,$jc){return"<li>".($E==$jc?"<strong>".($E+1)."</strong>":'<a href="'.h(remove_from_uri("page").($E?"&page=$E".($_GET["next"]?"&next=".urlencode($_GET["next"]):""):"")).'">'.($E+1)."</a>")."</li>";}function
print_hidden_fields(array$gj,array$cf=[],$Xi=""){$I=false;foreach($gj
as$t=>$X){if(!in_array($t,$cf)){if(is_array($X))print_hidden_fields($X,[],$t);else{$I=true;echo
input_hidden($Xi?$Xi."[$t]":$t,$X);}}}return$I;}function
hidden_fields_get(){if(sid())echo
input_hidden(session_name(),session_id());if(SERVER!==null)echo
input_hidden(DRIVER,SERVER);echo
input_hidden("username",$_GET["username"]);}function
enum_input($Ma,array$j,$Y,$kd=null,$sb=false){preg_match_all("~'((?:[^']|'')*)'~",$j["length"],$z);$Cm=$z[1];$Il=Admin::get()->getSettings()->getEnumAsSelectThreshold();$M=!$sb&&$Il!==null&&count($Cm)>$Il;$U=$sb?"checkbox":"radio";$xa=$M?"selected":"checked";$I=$M?"<select $Ma>":"<span class='labels'>";if($M&&$j["null"]&&$kd!==""){$tb=$Y===null?$xa:"";$I
.="<option value='__adminneo_empty__' disabled $tb></option>";}if($kd!==null){$tb=(is_array($Y)?in_array($kd,$Y):$Y===$kd)?$xa:"";if($M)$I
.="<option value='$kd' $tb>".lang(111)."</option>";else$I
.="<label><input type='$U' $Ma value='$kd' $tb><i>".lang(111)."</i></label>";}foreach($Cm
as$X){if($kd===""&&$X==="")continue;$X=stripcslashes(str_replace("''","'",$X));$tb=is_array($Y)?in_array($X,$Y):$Y===$X;$tb=$tb?$xa:"";$le=$X===""?("<i>".lang(111)."</i>"):h(Admin::get()->formatFieldValue($X,$j));if($M)$I
.="<option value='".h($X)."' $tb>$le</option>";else$I
.=" <label><input type='$U' $Ma value='".h($X)."' $tb>$le</label>";}$I
.=$M?"</select>":"</span>";return$I;}function
input($j,$Y,$o,$Qa=false){$A=h(bracket_escape($j["field"]));$fm=Driver::get()->getTypes();$Cf=isset($j["full_type"])&&Admin::get()->detectJson($j["full_type"],$Y,true);$Dj=(DIALECT=="mssql"&&$j["auto_increment"]&&!$_POST["clone"]);if($Dj&&!$_POST["save"])$o=null;if(in_array($j["type"],Driver::get()->getUserTypes())){$rd=type_values($fm[$j["type"]]);if($rd){$j["type"]="enum";$j["length"]=$rd;}}$Ma=" name='fields[$A]' ".($Qa?" autofocus":"");$se=(isset($_GET["select"])||$Dj?["orig"=>lang(112)]:[])+Admin::get()->getFieldFunctions($j);$Ie=(in_array($o,$se)||isset($se[$o]));echo"<td class='function'>",Driver::get()->getUnconvertFunction($j)." ";if(count($se)>1){$gk=$o===null||$Ie?$o:"";echo"<select name='function[$A]'>".optionlist($se,$gk)."</select>",help_script_command("value.replace(/^SQL\$/, '')",true),script("qsl('select').onchange = functionChange;","");}else
echo
h(reset($se));echo"</td><td>";$nf=Admin::get()->getFieldInput(isset($_GET["edit"])?$_GET["edit"]:null,$j,$Ma,$Y,$o);if($nf!="")echo$nf;elseif(preg_match('~bool~',$j["type"]))echo"<input type='hidden'$Ma value='0'>"."<input type='checkbox'".(preg_match('~^(1|t|true|y|yes|on)$~i',$Y)?" checked":"")."$Ma value='1'>";elseif($j["type"]=="enum")echo
enum_input($Ma,$j,$Y);elseif($j["type"]=="set"){preg_match_all("~'((?:[^']|'')*)'~",$j["length"],$z);echo"<span class='labels'>";foreach($z[1]as$X){$X=stripcslashes(str_replace("''","'",$X));$tb=$Y!==null&&in_array($X,explode(",",$Y),true);$tb=$tb?"checked":"";$le=$X===""?("<i>".lang(111)."</i>"):h(Admin::get()->formatFieldValue($X,$j));echo" <label><input type='checkbox' name='fields[$A][]' value='".h($X)."' $tb>$le</label>";}echo"</span>";}elseif(is_blob($j)&&ini_bool("file_uploads"))echo"<input type='file' name='fields-$A'>";elseif($Cf)echo"<textarea $Ma cols='50' rows='12' class='jush-json'>".h($Y).'</textarea>';elseif(($Dl=preg_match('~text|lob|memo|json~i',$j["type"]))||preg_match("~\n~",$Y)){if($Dl&&DIALECT!="sqlite")$Ma
.=" cols='50' rows='12'";else{$L=min(12,substr_count($Y,"\n")+1);$Ma
.=" cols='30' rows='$L'";}echo"<textarea $Ma>".h($Y).'</textarea>';}else{$Mg=!preg_match('~int~',$j["type"])&&preg_match('~^(\d+)(,(\d+))?$~',$j["length"],$y)?((preg_match("~binary~",$j["type"])?2:1)*$y[1]+($y[3]?1:0)+($y[2]&&!$j["unsigned"]?1:0)):($fm&&$fm[$j["type"]]?$fm[$j["type"]]+($j["unsigned"]?0:1):0);if(DIALECT=='sql'&&Connection::get()->isMinVersion("5.6")&&preg_match('~time~',$j["type"]))$Mg+=7;echo"<input class='input'".((!$Ie||$o==="")&&preg_match('~(?<!o)int(?!er)~',$j["type"])&&!preg_match('~\[]~',$j["full_type"])?" type='number'":"").($o!="now"?" value='".h($Y)."'":" data-last-value='".h($Y)."'").($Mg?" data-maxlength='$Mg'":"").(preg_match('~char|binary~',$j["type"])&&$Mg>20?" size='44'":"")."$Ma>";}$Pe=Admin::get()->getFieldInputHint($_GET["edit"],$j,$Y);if($Pe!="")echo" <span class='input-hint'>$Pe</span>";if(count($se)>1)echo
script("qs('select', qsl('td').previousSibling).onchange(null, true);","");$ae=0;foreach($se
as$t=>$X){if($t===""||!$X)break;$ae++;}if(count($se)>1)echo
script("qsl('td').oninput = partial(skipOriginal, $ae);");}function
process_input($j){if(is_generated_always($j))return
null;$bf=bracket_escape($j["field"]);$o=isset($_POST["function"][$bf])?$_POST["function"][$bf]:"";if($o=="orig")return(preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?idf_escape($j["field"]):false);if($o=="NULL")return
Driver::get()->getNull();if(is_blob($j)&&ini_bool("file_uploads")){$l=get_file("fields-$bf");if(!is_string($l))return
false;return
Driver::get()->quoteBinary($l);}$Y=isset($_POST["fields"][$bf])?$_POST["fields"][$bf]:(isset($_FILES["fields"]["name"][$bf])?$_FILES["fields"]["name"][$bf]:null);if($Y===null)return
false;if($j["auto_increment"]&&$Y=="")return
null;if($j["type"]=="set")$Y=implode(",",(array)$Y);if($o=="json"){$Y=json_decode($Y,true);if(!is_array($Y))return
false;return$Y;}return
Admin::get()->processFieldInput($j,$Y,$o);}function
search_tables(){$_GET["where"][0]["val"]=$_POST["query"];$Ij=$td=[];foreach(table_status("",true)as$Q=>$R){$ol=Admin::get()->getTableName($R);if(!isset($R["Engine"])||$ol==""||($_POST["tables"]&&!in_array($Q,$_POST["tables"])))continue;$I=Connection::get()->query("SELECT".limit("1 FROM ".table($Q)," WHERE ".implode(" AND ",Admin::get()->processSelectionSearch(fields($Q),[])),1));if($I&&!$I->fetchRow())continue;$w=h(ME."select=".urlencode($Q)."&where[0][op]=".urlencode($_GET["where"][0]["op"])."&where[0][val]=".urlencode($_GET["where"][0]["val"]));if($I)$Ij[]="<li><a href='$w'>".icon("search")."$ol</a></li>";else$td[]="<div class='error'><a href='$w'>$ol</a>: ".error()."</div>";}if($Ij)echo"<ul class='links'>\n",implode("\n",$Ij),"</ul>\n";if($td)echo
implode("\n",$td),"\n";if(!$Ij&&!$td)echo"<p class='message'>".lang(79)."</p>\n";}function
help_script($Dl,$Ak=false){return
script("initHelpFor(qsl('select, input'), '".h($Dl)."', $Ak);","");}function
help_script_command($Jb,$Ak=false){return
script("initHelpFor(qsl('select, input'), (value) => { return $Jb; }, $Ak);","");}function
edit_form($Q,$k,$K,$pm){$ol=Admin::get()->getTableName(table_status1($Q,true));$T=$pm?lang(39):lang(113);page_header("$T: $ol",["select"=>[$Q,$ol],$T]);if($K===false){echo"<p class='error'>".lang(90)."\n";return;}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";$gd=false;$Vm=($pm&&!isset($_GET["select"])?where_columns($_GET,$k):[]);$ac=(count($Vm)!=count($k));if(!$ac)$Vm=[];if(!$k)echo"<p class='error'>".lang(114)."\n";else{echo"<table class='box'>".script("qsl('table').onkeydown = onEditingKeydown;");$Qa=!$_POST;foreach($k
as$A=>$j){echo"<tr".(isset($Vm[$A])?" class='where-column'":"")."><th>".Admin::get()->getFieldName($j);$t=bracket_escape($A);$h=isset($_GET["preset"][$t])?$_GET["preset"][$t]:null;if($h===null){$h=$j["default"];if($j["type"]=="bit"&&preg_match("~^b'([01]*)'\$~",$h,$Aj))$h=$Aj[1];if(DIALECT=="sql"&&preg_match('~binary~',$j["type"]))$h=bin2hex($h);}$Y=($K!==null?($K[$A]!=""&&DIALECT=="sql"&&preg_match("~enum|set~",$j["type"])&&is_array($K[$A])?implode(",",$K[$A]):(is_bool($K[$A])?+$K[$A]:$K[$A])):(!$pm&&$j["auto_increment"]?"":(isset($_GET["select"])?false:$h)));if(!$_POST["save"]&&is_string($Y))$Y=Admin::get()->formatFieldValue($Y,$j);if(($pm&&!isset($j["privileges"]["update"]))||is_generated_always($j)){echo"<td class='function'></td><td>";if($pm||!$j["generated"])echo
select_value($Y,'',$j,null);else
echo"<code class='jush-".DIALECT."'>",h($Y),"</code>";echo"</td>";}else{$gd=true;$o=($_POST["save"]?isset($_POST["function"][$t])?$_POST["function"][$t]:"":($pm&&preg_match('~^CURRENT_TIMESTAMP~i',$j["on_update"])?"now":($Y===false?null:($Y!==null?'':'NULL'))));if(!$_POST&&!$pm&&$Y==$j["default"]&&preg_match('~^[\w.]+\(~',$Y))$o="SQL";if(preg_match("~time~",$j["type"])&&preg_match('~^CURRENT_TIMESTAMP~i',$Y)){$Y="";$o="now";}if($j["type"]=="uuid"&&$Y=="uuid()"){$Y="";$o="uuid";}if($Qa!==false)$Qa=($j["auto_increment"]||$o=="now"||$o=="uuid"?null:true);input($j,$Y,$o,(bool)$Qa);if($Qa)$Qa=false;}echo"\n";}if(!support("table")&&!fields($Q))echo"<tr>"."<th><input class='input' name='field_keys[]'>".script("qsl('input').oninput = fieldChange;","")."<td class='function'>".html_select("field_funs[]",Admin::get()->getFieldFunctions(["null"=>isset($_GET["select"])]))."<td><input class='input' name='field_vals[]'>"."\n";echo"</table>\n",script("initToggles(gid('form'));");if($Vm)echo
script("initWhereChange();");}echo"<p>";if($gd){echo"<input type='submit' class='button default' value='".lang(115)."'>\n";if(!isset($_GET["select"])&&$ac){$Jc=($Vm&&Admin::get()->getErrors()?" disabled":"");echo"<input type='submit' class='button' name='insert' value='".($pm?lang(116):lang(117))."' title='Ctrl+Shift+Enter'$Jc>\n",($pm?script("qsl('input').onclick = function () { return !ajaxForm(this.form, '".js_escape(lang(118))."', this); };"):"");}}echo($pm?"<input type='submit' class='button' name='delete' value='".lang(119)."'>".confirm()."\n":"");if(isset($_GET["select"]))print_hidden_fields(["check"=>(array)$_POST["check"],"clone"=>$_POST["clone"],"all"=>$_POST["all"]]);echo
input_hidden("referer",isset($_POST["referer"])?$_POST["referer"]:$_SERVER["HTTP_REFERER"]),input_hidden("save","1"),input_token(),"</form>\n";}function
file_upload_form_script($ie,$of){$Eg=ini_get("max_file_uploads");$Kg=ini_get("upload_max_filesize");$Lg=ini_bytes("upload_max_filesize");return
script("initFilesUploadForm('".js_escape($ie)."', '".js_escape($of)."', "."$Eg, '".js_escape(lang(120,$Eg,"'max_file_uploads'"))."', "."$Lg, '".js_escape(lang(121,$Kg,"'upload_max_filesize'"))."')");}function
compress_alphabet(){return
strtr(implode(range('"','~')),"'\\","!\n");}function
decompress_string($Tk){$Fa=array_flip(str_split(compress_alphabet()));$u=strlen($Tk);$Bm=($u?13*($u-1)/2-$Fa[$Tk[0]]:0);$Ya="";$Gj=0;$Hj=0;for($p=1;$p<$u;$p+=2){$Gj=($Gj<<13)+$Fa[$Tk[$p]]*93+$Fa[$Tk[$p+1]];$Hj+=13;while($Hj>=8&&$Bm>=8){$Hj-=8;$Bm-=8;$Ya
.=chr($Gj>>$Hj);$Gj&=(1<<$Hj)-1;}}if($Ya=="")return"";return
function_exists('gzinflate')?gzinflate($Ya):inflate($Ya);}function
inflate($Ya){$ig=[3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258];$jg=[0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0];$Mc=[1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577];$Oc=[0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13];$J="";$G=0;do{$Yd=inflate_bits($Ya,$G,1);$U=inflate_bits($Ya,$G,2);if(!$U){$G=($G+7)&~7;$u=inflate_bits($Ya,$G,16);$G+=16;$J
.=substr($Ya,$G>>3,$u);$G+=$u<<3;}else{if($U==1){$vg=array_merge(array_fill(0,144,8),array_fill(0,112,9),array_fill(0,24,7),array_fill(0,8,8));$Pc=array_fill(0,30,5);}else{$ug=inflate_bits($Ya,$G,5)+257;$Nc=inflate_bits($Ya,$G,5)+1;$D=[16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15];$Vg=array_fill(0,19,0);$Ug=inflate_bits($Ya,$G,4)+4;for($p=0;$p<$Ug;$p++)$Vg[$D[$p]]=inflate_bits($Ya,$G,3);$Wg=inflate_table($Vg);$kg=[];while(count($kg)<$ug+$Nc){$el=inflate_symbol($Ya,$G,$Wg);if($el==16)$kg=array_merge($kg,array_fill(0,inflate_bits($Ya,$G,2)+3,end($kg)));elseif($el==17)$kg=array_merge($kg,array_fill(0,inflate_bits($Ya,$G,3)+3,0));elseif($el==18)$kg=array_merge($kg,array_fill(0,inflate_bits($Ya,$G,7)+11,0));else$kg[]=$el;}$vg=array_slice($kg,0,$ug);$Pc=array_slice($kg,$ug);}$wg=inflate_table($vg);$Rc=inflate_table($Pc);while(($el=inflate_symbol($Ya,$G,$wg))!=256){if($el<256)$J
.=chr($el);else{$u=$ig[$el-257]+inflate_bits($Ya,$G,$jg[$el-257]);$Qc=inflate_symbol($Ya,$G,$Rc);$Ch=strlen($J)-$Mc[$Qc]-inflate_bits($Ya,$G,$Oc[$Qc]);for($p=0;$p<$u;$p++)$J
.=$J[$Ch+$p];}}}}while(!$Yd);return$J;}function
inflate_bits($Ya,&$G,$dc){$J=0;for($p=0;$p<$dc;$p++){$J+=((ord($Ya[$G>>3])>>($G&7))&1)<<$p;$G++;}return$J;}function
inflate_table(array$kg){$Q=[];$zb=0;for($Za=1;$Za<=max($kg);$Za++){foreach($kg
as$el=>$u){if($u==$Za){$Q[$Za][$zb]=$el;$zb++;}}$zb<<=1;}return$Q;}function
inflate_symbol($Ya,&$G,array$Q){$zb=0;$Za=0;do{$zb=($zb<<1)+inflate_bits($Ya,$G,1);$Za++;}while(!isset($Q[$Za][$zb]));return$Q[$Za][$zb];}if(isset($_GET["file"]))load_compiled_file($_GET["file"]);function
load_compiled_file($m){if($m==""){http_response_code(404);exit;}if($_SERVER["HTTP_IF_MODIFIED_SINCE"]){http_response_code(304);exit;}header("Expires: ".gmdate("D, d M Y H:i:s",time()+365*24*60*60)." GMT");header("Last-Modified: ".gmdate("D, d M Y H:i:s")." GMT");header("Cache-Control: immutable");ini_set("zlib.output_compression","1");$Fd=pathinfo($m,PATHINFO_EXTENSION);switch($Fd){case"css":header("Content-Type: text/css; charset=utf-8");break;case"js":header("Content-Type: text/javascript; charset=utf-8");break;case"ico":header("Content-Type: image/x-icon");break;case"png":header("Content-Type: image/png");break;case"svg":header("Content-Type: image/svg+xml");break;}switch($m){case'favicon-blue-0f5ce53a66b1e25395d0048da369f19e__6bb95962.ico':$e='AAABAAEAICAAAAEAIAC6AQAAFgAAAIlQTkcNChoKAAAADUlIRFIAAAAgAAAAIAgGAAAAc3p69AAAAYFJREFUeNrV1wEEGmEYh/FztCYBRATANhCAAEGAEGZowEUFhM2G6A4QAJksoMi2AYRlAxgcAUgthAS2yTFo5d2DDzbO6r2PhB9APY73z+cUn3+6qbsJcFGCjxlCbPHL2CLEDD5KcG0EPESAH5ArfUeAtDbgCb5BElrjsSbgI8SSD5qAM8SSsyZAbNIErCGWrDQBTYglTe0ZNnCAKB3gJR2iAnwsIBdawEchyRC9jompoYUe3hg9tFCL+dNX2ivo4wEcpTT6EF0AsEMHeTgXyqODnf4M489phC7aeGq00cUIK1s7sLr1DryEWPJCE5DBBJLQBJkkO9DAHnKlPbwkO/AMjuGijCGWiCD/iLDEEGW4f/2WIuA3qnBiZPHIyMKJUcVJe4ZHDJCDc6UcBjhqz/AEMSKMUf9PTA51jBFBAN0X+AKJEWGDr8YGESTGZ02AB7HE0wSk8B6S0DuktDvgYgRRegvXxsuogjnkQnNUrL8NUUSAKUL8NEJMEaB4x4/TG/gDMBOIUjRp9w0AAAAASUVORK5CYII=';break;case'favicon-blue-17e440832c1eac07527560a0d6f0d2ee__6bb95962.svg':$e='+<bATb3V?$so%el,wEIwK&mlYjGZ$a8-HGs8y$j)-UBmQx`Cf?>]C6?xmhS1<w
ZNSJl63"aZ]nB<rgk|tG)vC,pHYx;Wb2NVXBMxd!lQ=g0"!mM[]c*SW?7
E^]B7t[fHolNczfsymCW
CM~8Ult[(brlOx`71nDc
H;N~ybgNd*l6LMbRk;>%06rQeBnDc14r_7Z0b9uqO=xV5=22d05hr#V]F`V>ZD,#JA9[$XEV-*TBfz7Z%
rEJw!G!cT+[7>-u:iVU)N$iv%ySN.`._&}sV@FUIuH)%cY-0#OXWPT8t./3Oid8~R]3?9n;/Qs
Y2!K`1Q9d
tys6C=xmJfXFCT%0xl`H&%njK7`N[.q6jET6kV
VqmiJoIrZ#rsP~q0vDN<FH1w9l4R-?A#H:#onn0@0]3dNk3,E{<7r;2q
u)F!d(nhskXC~JT4N!~!g52r#`3hF[%j
oPE~ZV(W_~g#t?WXQCxEe7)ZQKGxei4.gu_R>pAL]HDZf!uSL)$_)^vZ:Xk![_HKh=C|S~PjqHcgjUutq#-~?PmA#<MYyg2R';break;case'apple-touch-icon-blue-f2a5f6f50418d7293b806faf273fe381__6bb95962.png':$e='iVBORw0KGgoAAAANSUhEUgAAALQAAAC0CAIAAACyr5FlAAAK7UlEQVR42uzSgQAAAAACoP2ln2CDYig9QA7kQA7kQA7kQA7kQA7kQA6QAzmQAzmQAzmQAzmQAzlADuRADuRADuRADuRADuQAOZADOZADOcbeOUa502RxeG0bX17btm3b/tu2bdu2PbZt21bY+6xqPduTSSddSd1zPyTdVZk+p5+prr51f7d83ixVOXXH55Yte6No2r25Qy/O/Pg7OB/4ykFO0cC/4FBma6qq2Tsmb+SVGe9/7f86zWhMFx+HQ5mjs7XmwITMT77LXe+R04WOdFdw+Ka1xOzL7vML7rTLTnd+RMHhW+Z01Owfz911izOE8IMKDl8wh6W9dPHLbsFCOD/Izyo4pB8z3EyG4GPJK5rTqeCQ2HgEGECGeL5MVHBIPAM1CAvh/AkFh5Rvrf/13STr0x/UHpmO4yXzn+7+3pev+VA0puN/fX/hDyk4fOSBkjPg96KN09KRN/KqbuBoSzsnGtNRz8NFwSFBDJRAeDdwCOsqS8/86Nu9gYP4mK25WsGBaXVNlpS8plORFeuP5k/ZkPbVnLj3J0e9Nib8uWEhj/QPvPvz8zd/eAa/54vzfOUgp2hAs6kb0uhCR7rXN1s0I42AN7dNDxxYY/CG3sCB1+wb66dwNLVaQ5Jqlu/L/XxWLPf+hvdOu8X5qS9mxS7fnxuaVNvcZtXcaiyI6IeDN9KyFW/3Bg6eTf4FR1p+0/ydWS+ODL3xfe6lsc6feGlk6IKdWWkFzW5Za+WG6YNDzF5bWIx1GQ7cUpXr+3DklLQs3pP9zNBg7plX/NmhIUv25OSWuv4KwFK7TjhsjZXic2dRQsYH3/h3OFLP6oSDP+rLcDAV6DM3jttjEu83P961gYRUDJ1wVGz8wtZYIb7Wn136b41bk0/phKNs+Zu+CUd6QbPAwmzeb158ZlHPECFVRy8cGz6nsdNuE5OP0kUv/gscCUd0wlE07T5fg4PFgQ3HCni5MCEWwrm8zScKNd3G7EEnHDX7xnGkes9occTe3pgz8I//HgDVAUfu0Et8Cg5eQxi6xT0wuQ9amNDSbtN0GBEOnXDUn1nytxEi6aQ42JEbIRo3hW/XCQfRDt+Bw2JzvDk+QgoshL89MdJqc7gRjqawrRzBs774ibWuWByvOzabg3hj4Fp/hGPaxnSJsBA+a2uGGx8rLXGHxPHCSXc47Vax0F8853EO1p2c73ePFeJO0mEhPDKtzl0T0raUM+I4XrVjiDhlb67J7vdrQp9+NyGdKuGwIXzWlgx3vcoS4/q3s4wl4mx7RkDlxi/97lWWuKe8cLw6JtzFIJgOOLI++6G1pkA06CyI9bsgmKRYiDdbF8PnOuDAC8bf7LRZRBuZwucKjps+OKNn4c1lOPDKLf1EG19beFNwsGTfGzjw5ug9oplPLdkrOEj2IfDQGzgyP/2+EMr6UrKPgkOkCboOB54/5jqntVP+NEEFh0sJxuQPc6QbL5n/jN8lGN/+8Vl54bjrs3NKmmCgfTg1Wl44Pp0Zo0RNBtri3dnywkE2q5JDGmikz4gcDrn81o/OksjodSE1ZPiykHrt4XwZ4dh4vMA1OTXPF3c+TcSY4ZNwOJzOT6bHyEUG2ginUxVv8Yi1d9oHL06UhYzhS5M6uuzuKPs00dWyTxP9q+wT/4hrDuUhGzF1YOP9M0jl3CuWJODdvUpWOM1oTAzUTwvGRafXvzHOpCmDb02IiM2oN6zUZC5L7aRikKpDKhfDA84HvnKQUzTQVKlJhpDTkZXPDQ8xDxYvjAg9G13ldKoiteYwu915LKyceR8juRcfImhoj4dXOBym5EKVmqxu6Nx0vACBvCexeH1c+JYThTUNXZoyKVT25bUdjCWUWkDobIh+elQYpRmOh5VX1HZoyswAxwdTo4orexzga2yxBCXU8M89fVP6l3Niqb1xS0/CrDSmC4U66M6PBCfUNLZatR5aQXnbR9OiFRzGLtkz9+x9VQymBRV1ncSzk3MbUQwExFefCK/YF1C6P6CUD3yNSqvjFA1o1vs5BHQ+PSRYLNkbZQoO/LOZMUJAZn5Dovfx9GiRz2GsKTjwdydFVtV3aqY3Bp63J0RwwQoOj2aCPdQ3ICK1TjOxhSXXPtAngEtVcHgnTZBFuIiUOhNiwfRTXKSCwxPWTaz6XEyVw9tRSS7gTFSliOgrOEyUYIxY8nBImc3uBUT4oweDSp8fLmIqCg5TZp8/3C9gxPKk3eeKqd1m6FDCj/Ouu+tc8fBlSUyAxAUoOOSQJjzw1QUK62w9WUgFN8IVblnESc1ropjTwIUJ9391wQBpgjdM6Vbu/uw8AZIxK5PnbMskF4ShheVcgl1ZRS2VdZ2dlr/l4/CBrySrEhyjAc1oPHtrJh3pjrzAAN2K6U2Jmm7/5JyhuhgFh7HG/ZNXmsDQpeAw0N6bHCUvHGrhzVibuTlDXjjmbs9UcBho7GshLxyE6RQcxoYgqekpIxls3UJcRMFhuCKSab901cCEFlLBYawt25cjFxwrD+SqNEHPGYp1WchYdTBP5ZB62rafLjK/4o0Aq+YVU9nnF+KqHxsQZE4yHh8YRFazhik4vCiqZkc3UxXtIFt90a5sIZ5WcHjZ8spa2bLJ608ZLqDvvLj8slbNbKZETUha5m3PvO/LC57HguV7BrDiqnZNmbfg0JNR3GWxkw+Gbva2jwwvPcgq7pezY4+ElHdZHfovXsFhiDH/Z2s3nY2hhNxj/qHdK53l2fH62PCFu7K52TChv1oVsCo4DLS/ZkUkZjdqPbSGFktsZv2+CyWsfjEtQH+mU49PM/aqpQsPLCRx/AjyNa2HRi8uG9eMNpXs88SgoNqmrt6nBCN5RW9NYDspt5ExhoUxnA985SCnaND7dGXU948OCPRcso/KBHtqcHBhRZtmessvb3tyUJDKBPOE/dsLgni+mNPisxrEq5OCw3D7z9VOahoz8dRMZmQpM2Pl8rycQ6oSjJkwMlEwlRaSp55KMPa0dZ+hyVqG0+nN0nUU9vhgSpTKPjepNIEiT7xwincZz1htY9feCyUUEVTSBDl0K+zTSfhLxKncbkx0wlNq5+3IemV0mCl0KwoOBGcuSJWoA0YtL7ck6mWXtKCFpKSkC/In3lw005rSrTzYJ+DN8RGUfhu7KoVxhZ0MWIUJSapJy2+iKCBjDOMBH/gaklhzKLiMBjSjMV0orEB38+pWFByUjpRXmjBjc4aCw0Cj3oG8cDBTVnAYaCx6oTiVkYx7vjjf1GpVcBhrpPnLCAdL9irZx3Br67SJEn2yOMWGPJRSqtIEqb4lFxykiqk0Qc8Zy1qykLF0b44mv0m2jdeoFcnmJ2Pc6hTN86ayzyl/TnKvmclALUEimYLDaxs0kdppTjJ48Nm9TobSrZD4effn500V0kCnqQlTcHhdzmSSbSLZI6Gk2jTSJgWHKPpzNLT8SZGC5XFH7sDufyLbSMFhxp1vtp4qonCxJ7EgKEc9CDn2B1IbALa220jgQJdmNBY8y2CRoK0miyk4hFEbn2oIbq/hgapq8Z7sPKnV9AoOMR1BkMjSF9XsH+kX6BoQj/QPpMb+uiP5qFF0TCwUHHIade/ZTnzJnpyJ61IHL0pgdz7yQFEskvlHTiEf+MpBTtGA4DevylLsJ/endulABgAAAGCQv/U9vmJIDuRADuRADpADOZADOZADOZADOZDjBTmQAzmQAzmQAzmQAzmQA+RADuRADuRADuRADuRADpADOZADOZADOZADOZADOSAe34f5izVe/wAAAABJRU5ErkJggg==';break;case'logo-de272eb4bdca9c6fffd38c073270fb1a__9d7e398f.svg':$e='(]^+JbP.FqjXYdorFxH%oTmn1#,Na[(-^<}T{`+Ahl-RItQoM;{4bK}l["$V3F6U&V6Ey@S8#w=t>3kaN[hLow+fWEUH+K<LoXqyEy6JupFy-JyK4S8q(7tl96;KLl/F|,Cz)p?p(B)[axu/4u77-)nvU
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
n9lrNOU01c?:p/5y16+Zkgo}`M)D6xm>7RiQ_b%p%ocllip!0).myrT"w[53iGRZBt8z<.d6p9("_WYy1v;v.xBx%3c3&hfawJgMtuxeflyK1.-:wQo"f_z&)W';break;case'jush-b3a93b18444da26820ff61746521dede__d6435f99.css':$e='%X.mPb3V?!K0u25Dm[994[Zg@N#Q)YOC=2R_hE~4)=>cbdia55M!*B>tthOR~qe;I4(JE7nkwCcvDp&`dDJK
f-B>jO*,sE/El7[<CeY-WtHaF_.kmd7o@TK)f,bmf|yor7&?P^<qeg8="Q1IULQ7R}m#!4#2";]EMU!F1OpTIT56:qJms/Dv-ge06DN|n`FW&*;(q:eY(cbjkU?x]gUap[%36X.0y?`E_>T!nUPeLxWQNBQNxgW
`cW13j?"h~G+;>ckmN2+
!n%Sl8/pL[B8
M3BQ=4I8t<Wims^cdx*+69AM:3(4#D7NrL5JU;F-nR)NxQWs7{b*3#%wvQ^i5"h{.StgdiF@(/BD"LK.,r2{$e&b4#$3x
`jO"8tQQ1CKB,TYk$:TIGnJ]L
WcYUpj,c5;pfH7%lebNsHuwv44..jP:
oZb0YJ@}qOxs&)5?s0oVA*
swu
jafJPvJQCQ5?:;JN^klF$8EV7&
J?y"=oJ8]]rh5ZK31++0s/h[@)W/]t4{Do0LZsyY]Ui=F:=Lj0OiN0Q{mj^W2%j7&7^of",u?pagL&"iw=-;EKZzP?gdKNd/SR.wcU#s5@q~Y}(R:VF$Q8dZ,L)ey.Hq2O=)g/K}w)n3hY`Mh>::q<VI?VE1nU^-^D/qa9JwIPcYCppQ/X@!6ZZjyFhzsSz&E^ke1)2F[wC7uibhCB4fEU_/mdWy=C$?"]4Jab*)fpfaP{Xx=d3$rPj<p5Yo$$mqn$a[gdSRbtw^Cd:8IL9Ep@:NwGhGJP+=>bUa%5!)#P![lKsPx,lj]_TA`s)}ui[?@K5<"=yhHX9ac1.(pCt<=mY#,*@Ho8y`eb"03`0_6Wx1`zEp[UTJnLo&k`Z*%U;u_AP@.ah.ussF)SKcuGHm@)HJg~D|K@x[(QcKT7b133MD8{9lxGHdF&gk".u/dfBj;$`frs[]W0RtuZ<]q+TW7f)<cBALJpw,4G-PO&to>Pl>;:<vNY$aUe;BxF$7V3N~j=h.gDd*D(06YWAm/nioW)g>F(ql[`!L/h`[Z(V`2J';break;case'jush-dark-f8dac59c6ad1018686e52a0e0357e421__2ec7793c.css':$e=',Gjwm6?!R"-YJmoGR`r@~cEv;#i.*-_KUyr[0$CF,>/n=#+liP*01.(73:+G.C]Ek+^-h&|hnGDq1:ccpxU98SxFh5MU%c+]DCcezAcUOWmDiL$
)yZA,ICx<`.i#E%U;lo*kf6u&LQx+!%1t]iP#G9;zGT,4U2"ha>hB#am`y1YU6$z!l#C%';break;case'jush-615bc0b9720a1de8edd2c6876a3495b6__42cd49bd.js':$e=',hk]`!>p9CvwpHP(hqgS0$0>DjvvnfNtZ=+9nN
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
h;"?;42+Ek-3I/kW>]Xpm&{U(IrI$Jz^Y`>bCp%eO/#-k+COv_UOa0c+@)PoXFVg}s@O]Uc5V(&a;YG;Gi8t_XZ4RcVd&PQCrlok3**odcr@H*o.En6&]b;+<3o3iV3?:ib=pX8anAfa:LLQ4Xn3Ag~Mun9yw?g>OUm@&0_6W>fal0DG)_iYGt/F+4C;VU:=EnJJTd<5*]2t_UE1`uOZ;.BnQB5h`V4Xj+nMANkNWB%U,L;xI"b_CY0<i=fDHOaw>9W:Pw=<qUXC|L,A6V&QdWMGd^pG4vVUX)@D|,ye.<d]Cey@V^h8eqVoi]^>SAJ-!=-l(<5"NaRg/auemy%/(<.yE,>)^14g)n2t8?xa1__)`jFPemd!~;yfZ`v5mUOE./~tk])7"pC`,"w8V/TygC%';break;case'icons-70163a2695280bf75edba563e7b5471b__2ec7793c.svg':$e='!n1FChAWz1*tCrXP%
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
d%=.2Qw3Bb';break;case'default-blue-e7acfdb81453b86f081569afa115859e__9486a148.css':$e='(erWObOZ31.Ov9U"l9Hk1R@SJ5kdLhxe/CTC?saq5qsKQ*LB$<b.0;*%{a4b)qE6WmLxHcqkHpxcO3?JJQoKJQhn1](K+bXG]GdB|o^?CF)K2E"Ff?k_f`{eY5%[jxmk|UU
_lAk[p~cpbyQFGd0kRI@7bRfIG~^Wp7

nro$!N`iR1@jK~I6S0kgrGp)jhrG?Tj1MM/&G}xZJA()Jw7
q4),:%u_@hVWM%S5W|qtF8FpsmDQh8dM&$]:lP7s
&wC9`H/8~8LoBvYc"!yr{a25$MY5|x;]dL{on1(b;Iuco`+M:DyiJmbIe>~CIv7uvLOHH3za|(V&cW]S}k~niv!EhCuO[lMZ9Df[l"?a<nLF82jYRy-
=-R@oE`VRy{250E41,y[:K`V,xS+LHUH/Fbcz>
Dkv/<T1.2W#C<g^>o/YI)RaGHzv:Nd"3`MoUY%?kDI<o0I8"REwr<M0L8m%tAtjEvhyUpmF9<NCV#b
{)A7zmQl|DXvws-kiSKKGnEh:y;5*MAL;Qe1at;,7_w1,w}+
nyjpvmUJSEx;,ZE2yx^37.kVMqF]H^/h5rH!y&n[H"d!maWQv}May4S-]l9Dz&6}ymyL54F}4mtQuzqMH.tOi<t;yvny
!X{h*A^]YiSxk9|z!4/@-tVcdJio"Zcz)Mq,bm%N"uaSRx.,cbTAHxZtwE8xSuzw;M2c850w:yGypyJz$>sN%_hMr=.a<utWU]cXk^R_7GLSKK)m~Y5xs_@qn&(1ptOMm;Sk%P
JQhU>SEJoN5%N/j6ln/q20.wq/W^yL7aB(gW40IaG
[,hzaVh:0%jg.1J*_th|KE@0:xkzDmye_.if`aRjx8yh-%tz8/ScdT$hOeP),:T}ovJ#"u)-&r)!d/H.4C#.JIL|4*!]f@"o"W85,GThIm.6#@glD66<cXm"*7AvSrw5Y*oDM,rK%yuo:[jfP=I@l$UE1vdmuZ4IiflhooIX0!@`+O.sR9-oh_l8T1Y/+o_[C7^`V2_ep?5a3Z0.!Ua$"Plx^fR2Ekc3R8Ig$2J);Po)&vL<m7$@Agn#V>PHpk$+0tR+M%!mrU_C$`V?b%m_UwsU&5tkl8I*sN0g4}^bL
n<M].,"
YH^cZfsV#x#d=@cji$?
qL_2k%^0M^V4g-"M4ejS,7BdxZ.o"MSN_~*hg%nu+a0S4JQ7[(3*r(B7npr(R|
g
ztXZuu&R21v"9/GlF@KZlR[I$uwBxPal}9L)k*"xkBB]"eo
qAoD1rxn;<j["5O7iurKtDo]I2)RY0BXa[MJ[Rf>OY6Y2jrZvvmfwRDFU(#.s2^r*1d4vX.Yr;!"zj`J^fQrl!*"":k,fGZ`[2Rf@AO5S[!u~8ZRabbEtSN,/UF=3*R7KQ)27$ZQy<S73If]c0g4PA1;>uA!~!wy/j~?_d-e}$"O23]oo>ulvhgvSp;V)@cYK-[$--e^
UT8se{-ww#8Re=N>N;sfOMUVnW;ngT@iVAPcdZCs6oa/OVX5C[,Kuf`FGlh)HkTG)4[d0$*MG>45GHH-(yO#1C<mUE0js,DHO&4^dJ+{5?P~8wHT"s*^-vMIV,)CLlRqlSQ:K09Q3:O@H}hLDV`t&s)AEY&qS^<Yf>wlcZh>7;o1@/^%U=CSGGC4:uh-1KUs9hE
]+S|8?nF$&@fm"[M@nWSyl=62P%vBBERB7rHy8HL.JZFT
^Se%fO2%1{<3
Pp4:!ntgQQ:6Gn.+,t?J4b1A.ZLi"o<3L
w7iX}AUy;M.eX?Sw~&{w9tu^Z3hPX7M`$/W7G4wW4m.7@myU{+aQ.KL[QmQr=A"]SRWN4`@7^jEf]j}?^`}+:QvZxM~?gRl/GC=sBSdv.1Tt<!JAr]vt^qFIBvb%qI.^SS
]8F$+>N(yTL!!
DGn,i@ad+^jk3+$R:cBI:+em%::D:~F-*Wm<$a=Nn"f/ml5a0TS$[&
!36N6mG%B&OQ1"k9dE0SO%@alt
.&:Z>PN~S^?Oo3sT`cSW,]5ht`
~dAn1PC^B9&Zj[zvjL,ki[Y><Dgtn,;Q!@D#d4!FhgRbM6~X?WHo@c+C[azOxl~Uhc2e:.n.x
IQhQ$Z72v5U1~))(tHDT9**%oJXT9Kvhu.@G@E+<K1F=]"jH:w{ti
,I,g^+pCDNski+x$pOmlB=lfg
@H{O.v7+x_1RF;f;9QB$+Y-($A.iV7H<%i)wC<s^Bc8(Lf@RIM?U</iQ&"<Ra_"(13<`A_P/StqHr0<9VN7WQ=JG42V={"z&
#^-HQGgPHcC8CJ:g%!O$^nhpoX=2w6RTh8FnZ-J<Qn*42{Iv?!3Ji
InV&F5(=d4VkCix+o<k4
|
o%lv<KL74)/oeYf)2n8/S%A]=mk!IHz,jsLSJQp>,:D64>_@4O6
=(YO!Ca-G
y=[-Vr).hV>6v$kS]+odV)Sw"@e>e/T:+
Ah^HzSa/fcO6"S{cD.?7:v<xE$]`Ys^$JVOg:p*6>R6y)amCx
<p].F8zY{Mv7U$(jUXktcSVJsm<4O9:hC0S]kawh>x+L1]`I7`Z8K,PI^JU&f;B.ANSs=iX*`p&lx"_cGwW7u*$x5x##&*-qoqlAEc6s]f}IUM8BTk
J5Szk-ean)7`i^_8G:Ha1|+^CEGl@0i^3c,*v{%dlc95HE3lY900s9_yCbz)V;X`_|
pGU1
fB9WmaB,lqMq".Q^Z[ab-k4lM/nOI-;?q$uPj@w.Nb]H&}L&v.lq!iUyrC=iL$RLccA#rJ
39}#^
;*/[Hu8dNKvG|=M#
Qs*X,v1e<Z]TMGqTLTG?a@m&O*A]37yAU*^9Q=9eqyf,Sh^$O.xiQ<9ii-h90PTfSU05)<:*pE-faMVJIs%a@Dh[*4c:V%^Hhp8;s{KkD5L>yFwQc"o^v,Q`Uj!iX?mclF)
v!OXQ&(0P|X<;N,zN
d.0)-GA;j%R88EyX=]i_>yl1=tr)0=kd0W?f2V]WQ}olD"E|Yi7y(ZuIoTVqMh5{T8P^h=eeGx
w":eGsg&oZ!ZZZ~9*J[m8-P.0"S.pM}><YHBVVM>KJ"PL,7H0Q=C8R&%1,EOtpFOmN-WxBt<*%{;nxhb_MB*B/6xtxP.E3ulU*IoGH&NfK<FY0],ls9[dC_YT@N-gc<gdS43_=IoXxRPkBuJiO7yymH%kNzjK2v4)Ma)o321=YQqYO6d:Uv(-w]XsLB[X>2Ojmw:@M
_xymcdZFu(M7;NGv>Z>H6/jz?&=!E]sp=t0&X0a[[m)<A:-WEr)hYJ^v;XfN/_K-Mh3adXT78th(TAkID]Tk"Yr%.:OACI6+`t+)i.?^Gg8|[3b0?I?H5LI^V8nL;&115C*302f2.|1A0dDvNGm?
I;_NB.15+Fzu7oJb~kegd%CT*VJ+`6/nQ,iu-)m3LIoe%Z7)T
*-7nwb8+@>v2:o}=yEoO#i&hT:>NO7$Rf@K%0kw9:=8N@3yPsG~9/S=p75Io6$!RlW|"NbO?1c@bxIQ=UalT14H1M4E4mhb4JLu#d(PXm.7oRY"?HA3fg@svL/gVnfga~t[9Rs$^Epih7uoWbg;4<3B-{d0UV9?ne+mH
q~w`s5;Nt{hT&R>g_2Cdv!a@$Stj&j3P&Cu]``3v@l=|3gAvR}X2khutH:Ey/

PQ)$sW3RsQ,`GWn//jr(~?+B2PQS{NeM/R@L$f(5}8q++P.p;kEgs8E1GJh"Z+es):r/cN}E.WGe_Ej5b(eDz7`Hw_1UJj.i]bzxVuf>:3
kHp6-$R?6Y&ejljcVCjB)(J<jmfX.Ycc-e0f><DUHe!@kK)bQxqhf*wh/Cia57v%IB.dk!u3;<I1->?K)BTYJNrs#>8Aw:nw8_cV6<s39.J,kk,`V=,n"Bh}Wx1.@aTx#%)e*iXh.3wU-cpxRg#D-2c
7&RV_
j^TOR+[X)yMVjfKG.~?Z+SdHH_FceuO&nQFs:%HBH
v=_pv78)bVQh%PI6]8I^!q]lusJ.:">}4h&,s%3}hJOxIy0@PqVG)~ra<]U[/qf#PP#E5B>v(T/[U1W
Vt48XRBbF6*(H`w%#~#evpt0U
[ReEb$4b"]9vf$!GiXk9]-Ms`%m"iRxv3APl.=HJy)E5i&J1S5FtFl8CQ;bc#uaAHIX/m8N>e&2JG$uuxTD;vYJ|Cmdq!f:j[81q0ryjMT(JF6DjBFvh[
0p;O:UgUs"FJ/y
,nBc,A{*WOeKTuC+;nHe|)&({I]i"rvDmKA33&-fs3W*zij=SPT.G:Q=w0(>cd/N25Ig-)ZhWV@;d?SHY^:$69(Jz2Q!st~CE"n4.+S
uIMdG0NG_W8L$7ZSSgBY(O_5a)rN<n?H4f.EkvIO?w10t#,gL-|!8g$8;EFY~S2>^GtoI1I)N0:GsE:?C9S"#R?>#!rtdA5@yNY*kb$a?YPR8DhqO%
SLOz8MF$7<N6)]EPXQV0Aht/
tVddsOVC~#9d>C_[?!7tE22B5F>f.tV3l`tO3<oCxln_<fw93:u^9:-3
!suVS)J$NS4/x)Q{,)KOY3?P"eD_dyL4i?G"r;[zA{,fRwZ.MgSo7/!JVb0n^k-qdtBj;5Q]5:k^<A)V%R
2ccc+$mC^PpJGe<%cJW74do]E60y/`D6UP@Z:"8L`grXV.YdQw*6r@SywuN)E!--pOOP^fd>KP`tnb]=ap/&q13;6yIS>?P0^]LO{;p5=e?_#+XO|(%!triuw?s4}a>tlO;UYY/MZipQl?=prYl%,=(^%aB5dw1PXk9/t.1.csqw%t(_.Ha
Dn#?@er7hFo7Al)(ABk]Qt$&de{5}Y1Aa*=hnS6GTV@RH=hSmY;iRk?+XjQ92xbKIx_N:S(y*EmdT5o3FED6
w`.H8HNXDvIr"K76$H#lTIf-P~Ak01.vony:qA_B@Pmc%Fbj224dilaV".l|yG0>-].J?AEGgbR2.*9)DcAfYyW^4@]GSJ0%>BH(O?m(axW9>#[:KkRW:>Yd!R"_Y(I&TDj6rcrMCNOy&sd2wYt7qi^ZEdUfP/Ga*gg/R#5Eqn8@a2M,vd%r+{Ds`LN7BZ/;jZP/(6&2.0;2usv(^/<,M9*aWBV8%d1I0p3M2Y=7H"pL@vt}%~d6Y|b(:4mE+Gc3fs/<_?%7X=^(OCH3BClZ$r:@
:(1P7DhlfSax;&3Y?Te44A{.oedpw;pcE.44=Z-kA;)Q[bM8@B;n|Oe5E"X18?mQR*sT~V%@+m+x-&m(,+vD9-7sDC9W$Jk-GD$WmDap<p/i{V>6/)(*xTSG!#"Fp%5-LYYm&(*6&&IOPWvH$>qc$e-4M=.8L(0&-nnb?lI(9RHUy@]jm"`:/w6M.F`HQ3%
&Vq"#;Q1+fDK3RU`z#O:2i<-Un
CKA4n]"j9"IV$uHbp9
-+D_2EvU#D8bFgl4,lNHByl6"Ty*2LljcB*fHs3<er^SN
Zrr;Cq@ovs-r_PmDO,}Iu`Tvl/n8_kVm2cD[
4Am7U<u*P>?XMcZs0qLFK4jHJ?6k:J^AcoRm@*?{d?Q23eS:#lsYf6w"Q&9i2Y<"8AP`roc-!n6x>])S/<mz_74A*2UHXLH:Oj:GbO7DU{IIRBgWpvZk#gIw=_e"uBA/(I-x.IMwfw#l$u0-:$P8-=0-)0`~hjxPtlkFcW]z+X]p&jb4I/;:YOUUk+2_(W/QNftR(zuHs[-S[eosL,6gnc%A,
w"41x/t[u$?Vs7mZfb,hH2C-`7?
X{h.m;D(J/]liH6IsECa$PdFk5881%P{^d8^pcC,fMyv&G+=G[YGeS[0.E6N<N#rr}*L!p`1F0JHM6o-h,SYV&]1/o]APM*6XYEz8@I@gso;>9NFUk=K$m2JT$Ti?lT8tD6E$Ct=$cIuEZF%C[7ZWiLpJ,?8xLs&A;1QgKV6gH18JBNoOdy-h0Sv)HpwT4KP"d;d/Uv,+-u(3!n&BYt),$WyBZ,/[jNrev9
-:Vj/ncCo+u_Hw]SBMkm,v7BpV*dnECBJ,!4nbhO3R1=;G-oN^]K.38CfZ3$rue%R<P_7[^ZwY6m`}[UQ:]{X2a^9_4>.S,U/5X-b)O&8Wnby5@Gyl8TAv>o#j:j_o:s*Tsud#4%r`KWZDgi"jG),uP=n!w72ZD{)rb;fN*aI_c6Sg#(iWgPD
foxX(]*AfJOL)wY>F<>#e>Z2Q=ugG>B"wB]xG&ZvL"c}19.L`BhEOq#}$cD}4F:1^VOI8,n|[]c1PE6pJrRD[Wqg$J@&-}2/u][@.0?&>CB[vkd|DYXl
wO%;/wMYPe6/.Rvq#!T))MNXhMPT$4b2KMb$ErSq4UuSe2{_cq+[?a+?foG&;N@Xs$bwQa.MV-,r{+[t:@*]@Aee3:FdZPI:WQ6ZH+R,2k6c+M+p|8YjMT(oQUqgGdHv]z#*PR;dO8DaTd[YlB3w&>K@^M%,uGK$LUb!2X@MPYwK=PR$8dBrE7P=iSV?G^[(q7*)|O`&@*"[0nPX42Hv,Z#7L+&2{vZjV!W]w1g!u"G1WDN.`U1[x$)]D:Hocpjd_3mtGU28G/gy`N>3,`.*}ZQ^v_):1&_fR?M+Ap?"(Q-r{.xB%gI`l"D>0C%&As
-[lKw7#xHK:?tq&y[m_PiGtTVj,n_d=`$[W5XSGXKOc_r5@!NTc3.J*2w_N5
JRtklD.<X=@d72<SW
_E@JUp5d.I#_&qTF=]^*Yy$`)R,"h=A^;3_5$HG)t.)z"7!z)h?T`I7m|BT/1^N!2)J":7/p8h/m^D#tTbyb/RPLnjX6L?wH"`Y#VD9@<A94}P1VUd$3|ro%ytN.<!_q_u$f3CTZ0/t5?&xT[;]JGBt0W38F8C;?]tqDD&xCbr!lzB1$go^#D,ha^6e
x.~19m4bX[=u~&,Z*<doUM:wwZEY)@2@w:$UmM{>A$FpkHjcu.d7LGQpHuR,u:h1lb3+f$i*YdJRim$$]>4f~d(`Cj|=%rG$K`/$@Qs&jBv)?3*>8EfD/pgES=D!a?[V<i/rOW_3)O&8bHTor`35;R))JF,0drNtG72$^;8S#P_O#kWy2WtXiF`p9guFNu62WjRW]IIlBj78{j"IZ3EN_8X>ZOb#aondG08vTd$1FDe=`gw.l05
j:oi4&1gJn9YE%/aC;<Hjq:L:X5fdiY2wptxoW=wr&Lr7k=M]7OYwO@E@UZ5i*qw}MU/]>UrB:+3#;fv,O+rY.h!K#<(}bAKsD]Tw^2c7ZEHp<j)!7$Yi=@;7y&r0*%umqGtNq{N$p^LmMH4WDkp`81r20Z57.*0d2%q5oM;#WOkS#:o+TI_x;&0sXC?6UB"XWg/SN$%K';break;case'default-blue-dark-1061ad7d216f143e3626560b92b66061__747ec25d.css':$e='*O{SB7nV?&/MUNH!{Cu5Lel#Y-XFC?m<DF
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
s?[:Ep)rpcG^+CWaw-,2&0y-MkHQ#!:HNMp+q=GCO:UL:z"+P]23~)F!.KT@<L.z%)^_SQXy<>/W]]_cu5XiVEfr>f3ng3,IawA';break;case'main-0864f21d8576870afa2ea1b4d2bb6be8__bb1d1ac2.js':$e='+`K]`nsZ51ptW"r=6;m#J-HDqh*t)HqkN./vgS8^BaqT;T&lP"K2r[-"6#xJ#8#n

sLT"FDaX6I;p+8kvSK!
mB,]gbG_CuT)T4Igth0`t]
BQ`i?<=@ZXE:0dUV
aP:EdIcTMyL@^
Bg!mvRN0a;[qP6vv=c}W7u3p85-:g-z/G<EBtou0-xz;K2NV`b=j0a^/"oq01Go;IhKpoR=8;vvm#H^<l56OgDRx;l05A
z-2J},/_qxc7vq^UhBGk!RWj5V-36Hk"R0ar:vX0hL4[YVCZ#hql3]PU[-wKtP2#4jQE>g&DLQd9:D~09u%hlcD9=KKY=FH`i<DGo?H^2dLbJepM17taqPU?D;f!5b,s[HgRD7Pki[=bkNbO"XSrN<wGQ]xj)kEV]:4NsXhgwqn")q`yNtD(U@6Z[p{qgM8:I@EX:8d9!mLnicAX>9RDYTlxO+"ZI+?X)V<o"-hgKJ3#g5Rfb)fmdNoU]Tsi8hqIUoQ:kG@omS-%rPex>fn+Qa#,mp15wHTEzxr7RsouvX2
=;YL&Z7(865F<a5u;K*lY)0.Rc(0#g5i++yQ%@;8/iJ.K>Cg+9Cr?0RnH/kaI0gHc/IoAh4=OV.yzV5q+OKOk<Z_{PQc)q"V7+R(_J{NpOBe0@!r^-vLs-?^<R9A
q`KKO-KtqGL|5rtlBiQ,1TuC
}%~59KlWkN"/]HPI&M*^[`upxur<Q%yBc;A61Wt;C8:ur7~<zZ~8z/5f5L_T1Z(/_8CtbA[A
!P;I9EgmAt6/B(<nfY6j6d]<=q<|)x*8I1";Y<)5[_=q:,_;f3TJd-$?G+aZMn$";9c.TEp.<":#fm>E[.lU5O2GuJD{V7N{g<Z#RJe9wfP8UWrl*/fhb!L?v~gg<<:I#6MHb<jG/b+mh+U~[guGLi%`,<@!"it@w=3nP;EI=*nr^GNeCLhrEdMu/#hy9V;t=2y{VRKf.gN2!Khcl)X[%%,]e,nyfk<J6X^2yaw&4Cb?,~&rf5_aTfJ9xXs9Pxm`a=Md59qt??c~_JUeoG.v$v:3,+t/K7YI<lU$:t-&F@Mf*Fk<Dy1B*$soM@/#BfT}<G*0sZUAJ/-$Az)_yzaU2KrCh6hR"^_hu$C%+*eBlLpbj2Z;8!Tt%C@c*luImjz$
S;YdJ*NUmk(C]>(^hIAAz&df;O):pcz?wj
,+.isA%:im=X,l(0t<^#+??zL?&
cJ:
y.9r"WXoIznE9o8OkJ>pn7;e0),QZLCi%@tIl>T$i.vo[=n>Ql/%S,yi$6_V37DF-".ZCC-BR?xu^=2Kh1WW:B0@pBGKB5%EM3g;hXEtTg_^2E*Y>>Zua<h:%7#$3}l2A&]t7B=vN
_,=C_L
ndf-!`km<F3s]4m*
&9($7|oYu!;8EXfKAgP?IWJkABl4-Ze.Df!I&!S?G*r,Kv[$kIGm%SgO]iFL)6nl(X6GFKiWU3iVk(5Q*A/vJqjNc@]"RQ"X:H[(wjc%B{qQ3wPCvb3Xs)Nj(0Ln;RUfLWb_JvmsP}!Zsv=U.;*:_^[[0v)yEL;8,PgbT
*s>ym{qzP^#NC!V^GcLJvM%jK17#Co
L#Q-:T89vSgYk:2,65NN*w<k@RrcB19O)?88l?EhYnx)Ad7y0[a0)yr4VNH)}g8icjx:~x3ZB^{!3nn*no#Bo%/.q"P%q_+$[8/pZB66.1!!K7+PD7OEKEkT:07bcrD&ALR6
q)R!@`D8VdHw#YfqK0t1m|`_+~+)8lHa%C38$[$H_vpMQ##5n5!SX,wRYE#zCbhX9<yfZ8,matR;h`:NBvYaN2KcH&[9C?o`0>
4uSC5+nBeF{y&[6
)6<^V6$K=mC-C@oo==`#-p
rh,zkN?I)]HW*gYy;7
9u+V0b9Z}x3Gy7}n,u}bMp4Kr(:TkXKLHn*=,j[97hP9Tm10<+dPK+5O5fYdP.{lfbMNP"H1Ti}5`b
Pw4@Z
R|RnG7<g2D8~0H$B2<_X:Se9"9^dqj12C),BHxMK*LV75eYJPrnXW3l6cua&k}<)/f.6r)*?c:+cfRyBj
$cIpw{LZ1x%{N;Dbr
WiDiNjo
Ni?a=b$`[,7Hw60~Xfo
[w#bQu[#.XE87Vhn<epVpWalw&QsSCx:):N[8D5z$_-=Lt*Eg_IGI[4N@h=4/U
&=wf6SFgS.o=ghnw2h8K3^IX6Zkp6!}#=".R"iw-@2JBUZ.tkUH/9"3jrO9_/Sd)of-?j"$3Y^v(ETD2jwS@9L+r&0S1iJA-o8=#9;mW+t.,gx56o)Ly)Yqb_#kdEX3b>j#0I
r$>oovT"YP%a_t|hmR6:q2e>g")J[MRF5!}BFrP_bRT>JVfh]:[1A(HJvk#ry-{D0vX<L:=mr#qcD"m+-9mu66e,k^GERK6lh6Pn1:b@jx5)T2KGlLaH}>]&#p1NJs8utg1B">uwQwCW^(/x8$G2lUgjtioB("3,q,Yg$@:pB8X)>;@9mW:^+=r[N?J6JLa9!^p[f6.q~z!x&GD#=$pGRk4!sb9:te.!Ar3)P5kKw+`x(B]PGfa8+[HV?.^d98TA!qU]C]9(vko6cOV"W:R4HNC`
A:d5s8e;8-!8s{n#.H>QU5jSNY<ZFGy7,ePb
qy1`D%#3wUZsULj)AJ($P@)?5?I/utL"*va]F@HN6/&&~b[&}>lcT01&5nWN6[X&SC71,JE"NM=3ZMz[Ex!FTddatXv:@C~Oxe(?;q~%xVlS$d1Bf:Xc"dIRg2L4mHR/b,8/bRS*C4NQ!P)o(M3Bob9hu;p!oh=G,+dGvc_6CnPu<Bz2Ud^U21jMBA$nVj1avLCD%.O^y-{WsL0bz3OJ}N]T$@cV&mjmxn4iEj]tk]Chd3`x]w;=~vQftM%M&]Nr]-W`u0k%!4mewK-v[[8`B&"/e!hk,
F+Oo)WGNNLr5ZmqOM9H8pe@!-*.d|o5wb^wk]H:u=K03AF!+!>R)Uc_+O*_c#-]c(.VWuB^g%Szy$pv6v%gI&HVyi3]NCboXX+WPA[[Q#4M*<A%n=?~=x&7Vy`35*@o[+V2p)`(EcCrkf)1j)F:oP&;GHp2UmMg(CQ(,T2AfPEM:vDV`U2L]gn[b+s+Y1a(2]NfyyvfXJB>^jw{q}WEQ,LHg5e</u6,3V-H9.ri.L`SCXa0SwvDl~l[qPx2)P<>a$:5Vu?im2/N%Lx]b:mXu|/sUetmJB7fo|gkKVe9BRVYN?W=q<n@sCEn>fJ)df!r6T/MCdM6O%iFsSDiUf6XZ@L;]nL8cfe3&4MvN
NKT(b
^AB6i"Pnp0y-dqwrAYp,jlL7ACswgP@j
GIj;Luf/V?$(#
0b5=S]S],nhEANlu^<2!.6xfKm4s"=I];BW[cJV"9pXW.*=mU!Os`(k"35RU:4_2#ThNR0paTYAjiJ)%,8S7G6!CGI6F0I6H*WE%RLjjyGS;pNd@mVse|akOHH6R}enYHDV(G_c7XFrBNPo&%H9WTL^`93Er1<GJ+]Rw)4~P,#KX$,zvj!3j/Z!]d_g(!s6:)u`AJtNII5H.]eDBZU,]F(USu>-2H#G->FOr-[dm~.n`2j$bYI=isAi"o..a63^]5X=&yLgQHf;Pn)M)hXIVKYec=N0n0FgL`dZUQo7_
dQ74Jzv<^?MWGEw1Eu)co"qC;=0TDhm]9IuGTFl9<$@0FY&cL(X.5=V=F/s(PT2=*US!agizmBqOJO>u&j^u,APu>021>AaL]CIMDl%uA2(.E.;;iY#@pKtzM)Pr6t&/je"=?c"=J.Qjbt7^Dz6wd.i$c}q;YFMqkf"T]|Be6-"=ma)E[tx|M<Og.E)YvY)k6q#YyXSSn80m5ibUE+jip=*7kp7dIX$l:88Rm%JQEiQd5Wj@PB^K9Qal!rt6j
.vkyY(a<_D#X)
OR>;=UY&=-s)!")J&Sw.8A
wlLD%Fi_3Q!i&h/Hg
nKz+?^T:(YXeZvV:-kekK.jc5xIYg9%
gnk#fp/tmZDIfCBVbfunV$a:4@b6[^OrKXsPNM"8BqNP+FLMYf^jbA1Bt_=azp]A_p;cM!X7GiOXdB^]},(1{1NRV7<""5PD21V8{8QL+w"ujBN=n%+p<:DynEp^)YA/jZ`?y7-reT#E%4TY2T+v<mBGv2$I5n/!"7{oN:@_jpyYJQMfL$*W9NAp4]7O#g:cf1flqVc*Av*?bP.0sJ,>*oQ)+t5bt)vvQtt*vm5VN.rJv/e_NT$rq7P&qodTKj)
jti-`^F/;B{Uh/75k;{O?ca"4M&ss01t^==hZ3FAC[]Nae!X&;{hX*^Q#Y|Z-VP?4YHA%@?7H1|TMD^1*SwJ;G0LGoz#z4$$:OfC?m+%5)O8_H=G#5;lek?pH*r"GP^xnV
dOhNM=iF(A6>3G!/dQStXD<#IYSn>tDuB)K2T#Wjq9`>8`!VZCP&.;%xbdyl=,^jxrMnZc+VHw]DBZ#}hD>rh-[:dEK_uP&Es6OQ=aLv4f]kb?PV&K+E"ma`GM7i;jh;2S<TdCDBy"Ey,!oSmb6!?A.lQM9
>aP<)XDD^*
|]QPK39!!DBq]IacLvLXW?@*$3hWXFP,W!bk]_62^VMgS$ATl<w]uL2y]>X!A.$6ep[u#%`/E7}W>()2aGjS&G;bq"WtX&.$595Qy#zV16$E|O-ZQJTL%`t@b_<ml72D}1;eir?_M030$[}nv9$RHL4HN0,5&G!a)vLeP"juyr&:XP^wXwbS^+PRdv_?w5bS{d,_ZK9sQKyP3,SS67s?~9=AIMKp8de,Kvl,tba>4xx+%uM`SL,il"h@l9%
6,.@H]-=oiIYj-)P.s7oM#>q]^~-$Z4.)W@81g$hp#kM/^M(ot$EIP1-k5:3
_wIZy
,@M=:WkgJ2kwAe$
e.9CsIr-45m:^%X=c,2NGW%w;o7m%:E3VP90YY#Tl:G|YXd{FL>
K8P3fAU"2l_;sqYi?[3rZ:g=<k?b7|%jKq-q1z+u*2`TMO$B.aoLcSWl"E8(>6GJs?5XH0>eaT[pLD^B7Z7MJOM!j{Wy3<R-7XKimfJrLS)qfe4<I]DuYF[0AO]l)2a#.h&8KiSs_i`AVD#3CZOj`>w#(%#Nsr);;Lp>F~2RbmFK^wm`G1N8*-ex4^81WA0M,73>qxOJP18FU6>3P0feL4lte]HV(>:^>uK1jEEK1P_6RR%Jh<Eio[Gg/M9KOj-=RmV2UkaUjW"wA/a5.!tcG
/H^>>Md9g+OBTX0FscqG[WY7M"GW1QOV(,+vNWFKr}1WMWy@qGE+-zT`!}rx+5KAq]f]h2Blw-8V*kVW=.TPhHhb(h^kjX=P<YGJC~g<TNElC}?FKK2Nln&u5D.KQ$!>`E1{efAZ"$4Ui[gT-P_FAhwO<|7x0>yuHc?{wa.dQh/F[^u&QZBY6yOfm9V-gs>.gyLEOMazTFa}y%<@Y:
,K+aQ
%)Fk|X`+vjc--9jtPu$q97SKCnBs2p~1Y+`O)g6fUDddUh_<>0$MCdx3Xu{Df`Z1^]fA25;.TdK?"%tQv1mLr=c]CwHRgF(W~9/d=ujc-x/?"kmtzLC@bYn;p3O:
/_k[/@=V-C1:3u*as,wO3gPt&s`NX(o,U|Yh,,d)T9F"y,V2%LMQ_,-?Ij:!1L`WiI#T>:N*vm;o2yNLZV-XN~od)[,"
qQBA8jFEXmL(>XKb&PxW.LKv&JrVKR=u
IF!?v9<x/ud&awXMPe4jyIQgHfd<1[/[Hl8UT:DNA.8ih*tvB&TenC)4$cg0
*As%9&8?F>_>Vo~)v$7G9h^8$VG#-nwYA_-D|WVnsnj:=IH4V)Q*Q%pi@tq.8cn]TJ9DsRiZH&6Yiy6&4-d"6YW@BW0tces>E)!TsMP3:;K@&_$jv7V"wN
hcArt$]!K1I}&?txNOnykOj=3d/c%FW-YBtnN_x[v
Iq]Znq+TdD5H="3phEBW01-kY42yn4";1
TM.O[Xh?i,k?88RtoeSou`#ah!I
>_Iv8_l=V`9_W00YnX1DaR1bou#Lq(BCod,~)25PGs[^d1rPF=,(GjB7%Q+sf8-)VQ[cL?&lxJ4<xcK{ag#R?=?oVw17H48>@g%Lg75lR:c,cQ6PfqKm@5"M@#?U*Rl!4VS2]%8~TeU1hur;EP0`@V1Gk!!;9t[~J~g?3[(PX,=2J28aS|
<
oJ.gtF|)QIkUu<jls9&+0NFs*f)!=M^S9lsOb/<Z-(DckG*k~]]2Wf@6w_(4Iej7f:!#Lphm(JYyMS>Ruw0.kGLTD-cI:/3/QQws-LD_74II6<o2HjUZcmcae0Ib?vCF:p&u&(=XlV~2Lo)<^
ZNS3Z)LF/.d^6rKL!+DRpHE+M)N!?T.GAjU;Ws^^w;{dWqHw4G2@1K25=f`KO5
x<bPJ]UVJ>hk>a-zE2&_`@1a0o1Y)R#8HD61Fc>}>rZAE/Va-&^fW}s):6I*FvBUtAj~yX`I.U9XV:R)9EO"w(
|@`!0vR7V8d_^UFZ}@AuX[C-w,8
?U^[E])]$NdnWk[S&AKF}WjqmXIW
$C[0$U:[Ase0T[Be$P`1"rV|?!r;FS6X^MPRS^8l.Kk=7.bA054/]2
R(F/+wqck&[Jdoc[FuGkJ?:iE)X`8,)9{Ak`JN{CFw@%R
N`K?)Zqr_9gZl/5*w)Bs,?hF1*vy$1[kgU1>z:jFsnwDRxy=XkEo0,Dg6ImLa,lW4ujQudKd~9q[8TjNUoWrMGEM9EN9#a.($@ct"ELn}Q+ExWd;Fklmrk*IfcX[r#xw$,7XMH.re:%UYcty
R
)tqQ.H+lGARQH05ys/2ytSEHV4"^1mX{]vwAt`<V!+c|^C(mVzq;rGyC:+ZM2<2v;7;MKpo5%X;::fVFRTLQLcg:&6$M$.q/`SD]nHG:UUob*Mxds7<1N.8=9=D6PGqhuG^Y;{N5]JR_*`U=P_C*E2w=X|Q,/0?C
.gW+^Ab$yn%y5F:BYGPPTo9*re62w7`,#QPE1XCpay|(kS[KcJ`+UTQuJP-#rpYvgl*unfgm"q^9l[}@f<5Njf:k!5d;D8j^Nqqlh:M[u!Ru3F.1x-rMw,&vVBGbCW;7o[w8rD*pB/Xfp.xm)QwPb@o]KP)+]ZKW)*:ZTeW96*IuR[u(A-07qDA;8fDpnc]!@]7:N2?4,=Gb%&59R$/vsGcs`f*3)v$rXg2=WU&_u3^JkVL5^@+Z9<YMvRVAGz"TR:8cIw[kOO5
Emf)m,Vs0"N<o"T+-dNG/%;c)2diT(f<6PYgXx<w"Y(Hw)kfZ5%gj4Wc@&i.,*JP0jB##iEKp=@af$.l@%ji?UkSgH
nGwXrZQaq8
YK0VW$b@?c}h"xhwNcuVFQ+f7emNIYldu7I1MmcGR;^3r!(><3{G]Ug%Nx;tbyj]-X/s,3]OGk_0c,N.bIU&i:{Lsgb7t_Oh)TV!heu(*+(w>3Nx*(IBcAeXi#=o_4;&p0@:9Rx##hUA[NW&BOVeVBB,&1xjxNE.I8$IJw-+bX?mj$Hz(efWj:(,"%S1)pExsS6tl*z5@fa!=$D!}ikvY"g2n$b>(f}LoJm5HhWN0Fi[cw|dNZG*<n]]_QmdBU*A#h)%Nb}=:.B*^eDr];EfD:~J%5pe4Ir
p%)9}s?$;&,ons
,#LR5R[_MS2mnDEg@mKdV&7*Ns3aN4Z?8l5Og;A<58x.UUTG"O_u;8#i[3tKZOl&%~+A](_kj/ur)]4T-{@3ci6hl>,Z@SkjGoy[c[Z>o:whTb>n#yr&Gyb77`!~>4f`fw0D/O:@r)`TD9hQn9e2c(joXU.hb!G]b^A%PQDDp9D!hek{(5<I%3n3>Vnh%coeDw`==;Swa-3#t(Dvq|B1h5NZKydX0J+PB%laHY,)5
T|G)GC?1].D>m{R-q[j7TdD&"w*3.&X$,MKaZyNW8EW&oFR=5v&UU#3_PH:zH84{]J[SvICu"bH_(=oU_<MQN(&?Az:v&0(;q|@kh9P8pLF5-J;-;!%"@m0:[ythfyut$u-/)BIp&J.@088ofD+(&Ub*"4<f0251^]Ng(L$*KI.buRr/JvSTR{VF!4+>#D,ce>p,IHEYB9;-e4Pb#HE_/feZa[n//9bWQ8X,)(j:qz9Rx9,ScV=t)5Ov8I7e6rhPpVIQawu1Y-
#G(WcurXKB-%+>Zp0=##d3Q;Z=,4q+:NX?c)/.~p5m+2CL/[Z#F>9lR4?O,`lMLrX1?%@Y5#U"7U(Vief)Y[=.Lg/e@RL
S:10^d~>G`-=}3z,!*x3^)aG{nqR~fJy(Y$qtlx3_VN(4F@&*ujsLTm%"904Nu+C7S!Ho/%9AQBxx6|BOjOWnr7bbHow0.AM:
h9uu6C%IBAxfp$O1Lj>ieGujr=;:k.$*^VnU@d5&0rw_pH&
!Am;KwB<}l;&@Q}.~@]PdG3hI^_o-*]A+n*NyVgZe-bhA-ya^xv=c#B7q@_z)i^RdF?Hd-E6D
f4J9600xJWX`S#;A83=CURJoqpnJOy!idp7bT.$O6n1k]3F)Y4KVuok>_KY9J_@yY1u6+j:({3e`&?y&VcHhr0K_K&}R#,!
{RwfM;k>nMB0v[XTfQ7[=g)bp.M<gRzVC3v
6Cbt&)^G=<>O7#-1yrNTecM"*;Rj%v<bhH@Stf[s!`,(sQ>4hGl<kX/g{Rf;p4?vL!<YF_aSx
k`~${XA,Pl#7~*d?2h[UCm<FGH~U)gM(D@-rIETa?y76aY(DXMkvuGBOka&ib0DisPv8x7,2{@!@G:j!~beZ7nZN<ezXneDP0cw)9Y?<Z!x>m;(2tfY1(p}B$XQZ^tSresG#6kqi}i70X%dWFn)E/sVJm60l`i0(iVDi@omoSYA9s_
lG)9O:Hf!STmG-G2?%$PRGuNrO[z36"qF_obvL.DQ@y5CY56x3_^r8q6JwYNT$3-luQH[Q/oqd3q]Iy2@cRGuj.Y<:rt*}b@&#b?,~2;y-yfPYR}4%x_!rm6Opa+oQ4KI`vH/})h6c?^ll17!4<K&qXCQ~&;tM,64e:]O8k*.e(Qywc=#-]G/ed,C=xvwa>FK"--v!(/p69F8Gw:N~exsXl>1c#)Q"Qet7tc:xte"sU6V15n_7j^-G`}qrxxhCTCRdOiQtGlKYZn
2<l/SDYx1Ofhb=YErxMSrD~L#4k4H#5UyO+e;Dnu|toW0Qq0fY^Yxy"G6^:l0TGO3d+$YwB)kU]Gks5(IHHp)>ma]"ve@e_y=.9uWpDtSD5%``]nsQYlHt(V><:S7Xzk_h28jgw8dlHVtf~=v0I&07W*JP97E<sY9OyYB.1iuD6R,e0xA8bd7h$4vfBR4Jr_n&$Uo
3){5>6@^GCHsgE:(l,S(MvVb5AUW<$kb<[Qt`-DY_Kq*:5jr-w>h85VTych&3TEB.$GNg[uA4[fjO<X/r)teYspJ?/jUcA=+F;+Zam+?)4/$V#mo*i,rp
HWm-3h^.+>jrbqq?*@zYC.*sa;HYlre6q,k*/:l4JYJcJ,#Qoe~/6OK&M)YXJG4VFPQ$
"D,(lI$syCX4>DYp?%wyr[EguxFrgijCU%<TW5lDQu&=At[o+k?grOq]1LZHUYA|PPr/60N0%dW[fQIR<4sbJU61Lt?;#~Y7E7M,$,""uj&8>@_|/p&U/NMeh`s|j/HPNnsV"An:bBwq.QihnBh?F,nl0],VkHN(DS"]@h^RbzY/(N3=*1/
bh+JY/$Y>c=XHv7g[bl$vD9XWWSR)9E0K!OPWZ63@lG@tMpZ("NKISBIq87W&Epf+3DjGCwvFY]DChN
h",qPgD"5H)D-aO`w8C3?sZK.wda6oU8o`dSy,XG$hOY[%DRlpPr0@)6H=8YsRi6_YHQUmXOyu+]UFvrG;jq&^od9|]KT}4`<_sRXSA2]YnfnKl54%F!7}gG49liPW9tdX#.FTd:BP9RF@93d!6l,d9IA+F418GbC@pO]fTO28cN6!&@vTlp#v9[91AcM@O[n&#/uLS8UYKguYZ_xWA%7HM+IO0^rjL)f6jH
_hQndAx_u-{@`1WX;lrJ87{b""

c:3rB
]`i?thbI}xD;/36IzJ*OF:}NNK{cq:K;}GBAU@//v;F1wFipLb_*n>$XY*r9Z]av"LDt%z)?gC}m9JMVHf:#08jv#l>iGEE[FX]w:s/J7(XJfU`m85Xl8S`WK+@bM;5e%`_qH%go(YQ!`Jx?D7.$/1<XZH(k}t02eKla6]{saI&J.,
_2cl
ut]88#bT[4caT-7N7dden=AZW6lYh2egj_%3cu*Ikef6.j<
T$7kTm/WO3YLya9L:kJcOF4D6i;<;>EG2yhrbghDG?[PRq7u_;5#=I"d;:CDnI8k]JD4+9;?{AUfL.lq|Hw*(:[Or8gK&se=a6>S;s7acHz]+cMDRCz/Ah"8Lj"+d:h?$L~[oI84}C5DT``c1P}v]wHb3/#J*Q&#bekcDj
aHxX;t8{maQhDsjRt:&mVKrcnz5T!3H+lD#(wY@#9Sc2]C+Av}&{yF0*IfIG*FMKR/bR.h?.yf5Jkd
^B8um1PBR$)X|WF.<v
+BYcKyYY7
qeax#Tj6^4V{GOoV$2Sx$t2FF.<E<eu-?VdnYl=!sP/r:UQsXRT~`nd#oqH)N[(U@IaUd2a`y85@8
qI?CKu2#D;+MXvkOL8dLYY+C9:_&2YDVOXRU:y(H?)/``m_C$5Co_Tv01
5V"jd"g#&T2Ib$x]&yG(C-mR>XLdu.E$Ma,"ks4HViN3]pOm2P"s)*eG9E+89raj_(YUvB_J2{t.5)814!I^:+py"h.2@J]A+k8BQqZ|apy)/Xf8y$>!)#apI:
z$6"@RD::t2G;VX"M?2?*+TA,QwJCog6fV6$^p~G#6`GTH9@Cv=N)1{^gPTBx#YMRb$e2+v&>2/3JdhIZEB(9mU.k%Uit*QByQ(LYY`I>JvXqFgxwJycDc46E:G1UoiE~7X8LYBIbZAWkX_#:@(.D
_oUR9sb%I"Zx?(Kns6tsIl{;
owhYn
8C@16UG<g
n<v`iQAf`,4(`yl-Dxoe%mQ,*t
mvZx!=>2_7c1d5}/j>vQQe%uJ7W.zqWRY3s=OP~5#mZV>hi^p-6f7EQi!U`d^(/:lr@6cwEn*o)q(aXZDEtN@k&L
-;Ez)O3B=n4Wof!^,@a7Ee7aeGDNg<[8hzDIaGhI1btvdfel#hZlY!5&ba4#
7EBu"oMDER6We]^!Hf4:Qg-wf2FN~u"c_/.s/rOv=^QjqxZxAF{Udmb&
l>#~H52ly>jz#qO.T&c{+Jhj3>Q_"y/%#,
Fub-bmvZVf}VHI729<J#0p|20J)F#7FKXy~tYNZQ;m=DSv`0$2TbRk,aLETy]pjF*s82_=ayxA2@V.bCx_E/NdS5NZ:]9;u0~H|0LXw&d&i]x7/6OQ]H#*]!"Q92q;:1T9sL;86Uol.XsWCJ+ukU~n]JtB`&*dEL*[z>p?w%ue,D:"3uENAC1qT>wCNa>R:H#n4vW"rwg-)(#Cj$J4pHV_UKftRbX=1ZH;NMZ,ul#Z+AR!Jw5md0F.UZkB_b#w}8}lr>c!;"IiAfB=yg
j|TfA:ihL>R^N}5=]Ou@_685?$4oj"P3Yr,y`rYl1Q"wQJ$LH@$A9vF9a:/+I+2fwqPw^Gk|cjsJ/n<3SU+DjgJ3M?/VZG?DVXnn0V.yQksje*`Zg97&=}yVHGKNI*[w[y:gw<z&a9MY^%q
tG!VV`^D2W"H^+h2ro3a4vg@Kq`#sEpD[]]3fO8}EctKF~GcuLevty#Dg?wzM-&;Y)E.O)jU7Jg`5]
M*u02jjc~&e/]-exyg1[TSSZ^"{=fs%k/UdX5d__P,UC}2jr_r}0ecaXpxXGx;wa*%,!Vsq@g"+5u,buh*E%Vhpz%4#szgWVqJ
AfR2]1#fD*1(:m*lkH_70FnO>ABgE~[HKCP-/@VfXis.pN2(mEXHo(mO$#i3B{iamkH|A<)m<^sL+AOiZiY#pu$t8[Pa&,<9]gI!Ep9vm@wM3+lL+-g&Fm0lgUFQn0/8]$l@nOE|*8*^t5)&C
8itN4VFAT;hOQKlKk6n3oeJED9$}Xc(JU0C;](__V/b)ih;re7x%]L:?[CZ>(CT$/{"L%,!cowbb

&

)R6/JK,,b(ttLZukmZffbmu"=x;isBr"scK_pkO9"WibVbJJ("qctsxr!rPbM@+F7/Z+G8?n|wpjwlA0_=o4A
7tp.+v%vsNF8ks@HS6t,]kZ$`B9_~A:MA0f_s$Rm0/jF7awDc09`93}
*%7`y5*5k^F)<D&X|Bp>l"
T<.afb;dJ$8:uD7p5(k)i^_|xq!hLwz#QNp<E<o{ixatuE!mv"Vx^/r:l]]~/g;nqM^Gh)wp
>XM>JTnr1L`IXd%S$S%]^QkTCt`o[wls3pQ.sK.UNdx@%,OqMpH[TkJ([Zzir_VfbFu!$nPxb2&B.H!kYxh^QgkJ(!K,cFFS%mkyd@NH;r-ZRaQ_~FNev7sXPK"8;d[
o#H^O9:s8)H2/Cu9Y>};Pb~CpiNLs^cyxb??U]gKU8<+bE0(PSaWPBI1s;_3`X1(36Gu@d1j$T2Q01/vR$:9_
q?NC0&Z5EQ$^&Igl}<X
Hc{+-!fmbTx^zg]KY%9X:)<G8NfAb/T6G@cq*,xxm/<NG]6WJsH/#glFOF0gqV}4#ytR~Kc>-e=w-9:Xmb:KDTYwP[jMJf^xkY"nvDec]vr7/0I:AX,U!Y`"nuJUA-z,Qq1GrZ"LDu3:bltfY0*s~nDLuUT6|V[JbemcoM$3VmyHDw[dT[#[~Pm8R1#jaAF=jI}uY)4
DlF4}F#t1QtN`/xGZ*F#z,^oaS,Yy@eb&I=Map=r"%D;+=VKejP+&pbMNulXq>Fl{FmnM_CG~AXIu$L>$#WaM0KY#e!M_?Mo/fP3*V;5Kv~
>s+F*oxpKdOJ>VteNt{nI.G,Q*i],KByc&_C=3uA=M)GTr68&XvHP6X%brWb,!!h81q"^9%bwV="0P.sE6/eGKzHtmEiNCW*KNp[1IH]S7"!eYjW4.
t[1)s{$`$ppCPZ-4?ad#U583&)pL68<TqOF{3]bG8pHXe)Ia1L`zF9s!Swbfi>,W!Hwi*UtfS+`()Li`n2V4CRwf>27buOvhXM]SAU`,K*.e
TdoTgZiug^[L?WAe4`qTAAX1?/d+)Jeq2$7*[?BDKVsWDeiuM]8fD-Bt(LH0hR!1:-leN=e7O1,h}RR%m?sQ:X(M*k^pkG/quh;+UH{M_WdF9i/*YM_DN-)h+w(IKm@QNm2MR;=+!M[*I51Y]s73E(b!<[4s+RD0U#35ce7ui!I.L1L_
)eh=d9anDOUv1aIKi<M%u)V8e7:Ef@5r%:<(>w;*nx`I.XxLHBbBdJ!HLXIO/&*%7(gt05v<:6P]fO@^1sb8P)Fa2Ypo!W5{lzw-Dln}EOZac(QaoXY;Mv2drz$c06c|&Qa2Dae{mc<!UY-JCz132tU0nX
uvPfV]GHrgGN41d8e[pRw9SLLX9lqlp(~Ef75%L!rmvy=rVm%>V&}F2$plZ)E9RB-CJx[RC%rIHp><*dahKV3lkmyfc]4um;iP%L8..i5Xx
hJ]r_X}nzTaYUuNGww8
MBW+8Y>a_EVR(T$1,krO5XOL5UPn
Qx,~?A*Q321=Ekl<8`xE":Rp/g%.(kci/.`}+kn{.&Cs/^+t,wR27
O[olc(fri(anfTLhQIFKV_FE#~;fOU7ClwQ,UdyCn>^e=>@2p.MWs1g*Yd
Bno<_Y8X=2"?+1,:U8BC`yYt_lC
s@WVv?+`"/liD?GOkp72VPX8og`99Myqee[2Cy|SK0QcR+hEVbLfQK1vH&d)et3mKqpEZ/bZrQWlJ&TdP5|";y,tTF&lUrU8xrsAz[Z#Q1V^+L1,+8gubbk("`(_@$~EO#O[@@#I.;m2ge@x>l/YNxluN=hVd0a;SXJk+k7vCpu?h@0>84caBNj@*L2eS"7-NPt#R5tMt5Em=X1UQ",,mx
J?G~b~P6;;om@=/GrvP&DoZzGXA_8bgn)6/|Fc3[HxM*<ip:Oi,$8YE0fKb7<9)Bo`(xT|g6fkD;0@qIFk8"Ua>h]:c3H<3xdU?=2U`JH@fCn/N+/A`p5G-vDU,Yp>F%H@j5Y;LmoTb6STkO$Ck//zwRK"@2KSEXQIU3=QFblgFzto_FtjHJs[`$A9SQ3%x`#Vf,]9/".Y
HdgWH+:Vobd+j"I#Nb&B2BJV=/wZHKe[_*X4"HBkmeqqyKm"}W#C@oa<>.
$i+qx?@i(/4KYcXioX=MlJWOH/d7?T8UJrCp-EE:%UB#5Zi-GXM-QKNlIOAL%#er)R<gvmBi@okzF
I?rp$Yn.r4r1_mMNX7ug)~VpYYNZ=G"xIJ:>`rhG2[z#N{Oz[Dn!7A#Lp|!PXShas$/+u4h/9E2W7sneP(W!>}?nSZ/90uUtuY"1mfm9Az2mdn/1ZS*LYiGo2m7+ojjNP<MN^*CUgPG$,g&%3{Ze3.ccQ?HQ+i`:+dfU%apl!G*=sqN0wuI!`x<&+nb60]#.Qma{C:J#S<`zrr`p7>Fi)s"P1eCT27PEo8s]m!2iZ~1xr~#O#Q<]=uc$tau$I>kKlSi9RY89I1^62]Y>YF%0YVW!YM&XmtAvG0Ao_(HHELVooa-qQ2DUPQ^!kL_"?]Pb89rU0lwHDoS
L-!{1;=Ax.-m+e^&h~S4S:FVgH^TXdlzy?@=MX5(UhE_Z1B25(J)2pUH4xBeVm1Xog9^CxP{>]:U%b%0y$f]L
q~u4Su0%!sXk,.D*BF#/SCX[b?=iigdS*1V0;{rBE*HGnnS+rz)cb^sVY5xBjSx`9/cavy[&<
`BqvQ}F{ma&pZ/]&t4i(I]QQ>lAJw9r^7GyJF=o{Y;
K*;l.Y
tBl}4G<;w:ico/rL5"(]sCf]WJ.>IE&e+9;3Y}"Pjf"3v-+SDVDWbw_m,4CouXPOaZe=9e>(2>Dtc=y7rw&Q$AKnkuUJAP:]09mY<<KZ1c6"9w,6FoZ7k)nS[Y>$XZ2XQ[+/-,FI59/ZwzffYzh*J@&6:yP/uJ>Bbv*1N_]z(jy
;Y>]#8@}(}K*$ei]2IH?hX2K!<+!&7`O);T:96%jd=UuJkgOw6&J-m*Ds|jy3"Rem=F6&
wSR,%ULdEe#-"nyEKge/sd3+n=qEM3mzEqFS,EkqAbn~f/+}id(Je}1`p5t-yB9{cWL}%F;(/*C@d@F;ZaUPY33MOnNUXld_8yLck?vTl"Oy?P>k3@ywRH
H
"O}#+FCrT6]W&uU.@g<3.LMonqpZH@xk{J*:c3n"<H?V+J[4q5POBc@oI9jj28Y?6COlvp
@}Fyn4R9_Zfht-#IL&f-tYMROSF5r*WC/~g6:w9b:og+7/V,"_EkXov##9c["xv^GNP}FfyXULaM&emnj$.?Z(+/c<V6jU>
;K:T
M
jbmG[qsGz53JYY/q(eqVU$>#oS2?D
MA;I_bVA,y[6:)b`s_[*]#@:p(+>5vMd^R(o`Cf1i-vWm)}ZT56c(k:nd[nEufbCo/QU0I;=8cf[bhBF>)2)a8-lvYv?5?f5"AcC(*U^g0<$burBEC
M#"E+^*NXf1yKL5CWBAq9Q#Qu=8=(JIZ&Tl99=Zrh
NVrk*T
ze!a<p2qR@}ig;Mj%l].WbTA)/BNz#)m;Z5]@88t@?WYkb>loq2!wk"(xeOVaEw1wW]/a5B/,JTmji/F(mO7U=`>SpqRMXJoJs>1QgZ$P-a5o$(lL
p[P
#((f);jmWU}vNqDq&Kf4=Ym6!_rY"fC&nMM[t#<8S_zue?xRd$v@p!B#|ZH3A2]9I<k,t?Td]?:wGyL^@=l#i@c:JRN_)8oG1UrZ<:bG[FLfViMl[;^dtdd0tDrK/2/l/P%5{5sYiQ{[yQ*L11]<8W(["P}mRXg_d7j)}o%2z]2P-G7S3pID.6U6rCkva:bKt["@~e#.q#J!$=WbEe#@s!!j)xYoCW0inocY.Rwy~#4S|l(+~i*U#h-JT"r!Nd`&GbtgJso0*4Sg.yx>DR6Ny]Kj`T?-Li()Q[Iel5rvQM`Tl=/!?N(#.xh!0O&ojOEnVoab1GBnt`2f8OVm`LbPI?N8/x(uok=yq32SNJI<*l~:12iyw$4[-ScTl%DfRpT!UQ|pH,"cpTs9hnjLVU-Gb9cP9a[<E/s:P(&ph`%b#f^8#2*,pf8BYGflF^+i5.X^CC2v>Fd#]+!+1T0k@c>iISDji]V`=o`ysMYv]*imd5oKeAt>{?cd4VZ`KXSpVBCyK/h"txtQ7,w0kIeC=%"r{MblT`0i&r*q~G7l-BknaHRCVn2O,)h,j-"?D59rk-e3LAB5:E2YmP,UA)U
eFL_i)o+aAv+FlcC#$HaZ4&XyFFY%&#XJ=f;PeN
gg#K"<:$"SrGRnwc:AhT(/jym6k@F_lKAHSc2XG3j<dOGeMB[o,vxgHu,*B3vGfd)_}XaJS@~Fb?=Y)awZ&TyCb*pyD:ivuG!cN>D(pPC#ZL>60,mc~CuA<$;(w<<_J.FK(S}_%)bG7"8+Mv.!
JBnEuCg|5%,{b<1-;Y^F!ZwC3x(:5d<4aH.96!DoFscuU&bS(*dau{h<jKi=(3dO2H3NSS0<.U_**pQXjD?aDgIZr]pGv5ZD9?W^9E4*m?<i*IBm3o9J,vQa8ARB+*NQRZ$#`A,FsMaq*z)@Bv;My)!.J;N)N6%?5NNaW[eG>6CsaRrUej<mh%0<uk9q-h]R%T*<xJD
^[9gQoHy,3"Y;=f#
:&Qq@F,r4j6LL>C@}5Cb*"m6DP3T!Hcp"X7aWN)UE,3n{k*7b-mX"Y|r<!{!yy)+|YW,*$nF:c}rEI0bfm]r~L<e|;!ymOi#!hZ9EZFADOjQY@J6j8N2Z#)hItF`%4/J`%9AKr3^q`0lf??sFL0=XX}u{IKK"m;WW=}PK/E!%4,kl:0KqYLqGF}g#kUEB@|6JOA?=`v4/d`Gn%yb|oavI8W"EU8ePCjIA_=mLd0ThVx0>Ro8G6>MM6yC.$Us6nHnjQl9lN9Jhdh@UIN2Y:33}j1=1O6!|oCgiY5s}H_e1c3qUFMQF,8sG#R*pOp7Nr~;<*uK/Rfd=[XDq5v;,e1HU%5/OWj#nbAezhIFJvd4+[m%uQDZ&@xpbuNds?8AL&N`X6G]E^jI%.sJy!K66,iM/!"vO]x$lm97`Jw
s2DD=iF:Ztr3[+YfhGB7TB05;(r#J$dC`oFvCmpSN-VS})<2J2_"`KuZ)Li,J6_$*SM%z@a8o$#s
V%yw2R';break;default:$e=null;break;}if(!$e){http_response_code(404);exit;}if(in_array($Fd,["png","ico"]))$e=base64_decode($e);else$e=decompress_string($e);echo$e;exit;}if(preg_match('~^/[-\w.]~',$_SERVER["HTTP_X_FORWARDED_PREFIX"]))$_SERVER["REQUEST_URI"]=$_SERVER["HTTP_X_FORWARDED_PREFIX"].$_SERVER["REQUEST_URI"];define("Adminneo\HTTPS",($_SERVER["HTTPS"]&&strcasecmp($_SERVER["HTTPS"],"off"))||ini_bool("session.cookie_secure"));if(!defined("SID")){ini_set("session.use_trans_sid","0");session_cache_limiter("");session_name("neo_sid");session_set_cookie_params(0,cookie_path(),"",HTTPS,true);session_start();}if(function_exists("get_magic_quotes_gpc")&&get_magic_quotes_gpc()){$_GET=remove_slashes($_GET,$Wd);$_POST=remove_slashes($_POST,$Wd);$_COOKIE=remove_slashes($_COOKIE,$Wd);}if(function_exists("set_time_limit"))set_time_limit(0);ini_set("precision","16");@unlink(get_temp_dir()."/adminneo.version");class
Locale{static$Languages=['en'=>'English','id'=>'Bahasa Indonesia','ms'=>'Bahasa Melayu','bs'=>'Bosanski','ca'=>'Català','cs'=>'Čeština','da'=>'Dansk','de'=>'Deutsch','et'=>'Eesti','es'=>'Español','fr'=>'Français','gl'=>'Galego','hr'=>'Hrvatski','it'=>'Italiano','lv'=>'Latviešu','lt'=>'Lietuvių','ro'=>'Limba Română','hu'=>'Magyar','nl'=>'Nederlands','no'=>'Norsk','pl'=>'Polski','pt'=>'Português','pt-BR'=>'Português (Brazil)','sk'=>'Slovenčina','sl'=>'Slovenski','fi'=>'Suomi','sv'=>'Svenska','vi'=>'Tiếng Việt','tr'=>'Türkçe','bg'=>'Български','el'=>'Ελληνικά','ru'=>'Русский','sr'=>'Српски','uk'=>'Українська','he'=>'עברית','ar'=>'العربية','fa'=>'فارسی','hi'=>'हिन्दी','bn'=>'বাংলা','ta'=>'த‌மிழ்','th'=>'ภาษาไทย','ka'=>'ქართული','ja'=>'日本語','zh'=>'简体中文','zh-TW'=>'繁體中文','ko'=>'한국어',];private$language;private$translations;private
static$instance=null;static
function
create($ag){if(self::$instance)die(__CLASS__." instance already exists.\n");return
self::$instance=new
static($ag);}static
function
get(){if(!self::$instance)exit(__CLASS__." instance not found.\n");return
self::$instance;}protected
function
__construct($ag){$this->language=$ag;}function
getLanguage(){return$this->language;}function
setTranslations(array$Xl){$this->translations=$Xl;}function
getTranslations(){return$this->translations;}function
translate($t,$B=null){$t=$this->convertTranslationKey($t);$Wl=isset($this->translations[$t])?$this->translations[$t]:$t;$ag=$this->language;if(is_array($Wl)){$G=($B==1?0:($ag=='cs'||$ag=='sk'?($B&&$B<5?1:2):($ag=='fr'?(!$B?0:1):($ag=='pl'?($B%10>1&&$B%10<5&&$B/10%10!=1?1:2):($ag=='sl'?($B%100==1?0:($B%100==2?1:($B%100==3||$B%100==4?2:3))):($ag=='lt'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B/10%10!=1?1:2)):($ag=='lv'?($B%10==1&&$B%100!=11?0:($B?1:2)):($ag=='ro'?(!$B||($B%100>0&&$B%100<20)?1:2):($ag=='bs'||$ag=='hr'||$ag=='ru'||$ag=='sr'||$ag=='uk'?($B%10==1&&$B%100!=11?0:($B%10>1&&$B%10<5&&$B/10%10!=1?1:2)):1)))))))));$Wl=$Wl[$G];}$Wl=str_replace("'",'’',$Wl);$Ja=func_get_args();array_shift($Ja);$je=str_replace("%d","%s",$Wl);if($je!=$Wl)$Ja[0]=format_number($B);return
vsprintf($je,$Ja);}function
convertTranslationKey($t){static$ld=null;if(is_string($t)){if(!$ld)$ld=get_translations("en");if(($r=array_search($t,$ld))!==false)$t=$r;elseif(($r=get_plural_translation_id($t))!==null)$t=$r;}return$t;}}function
get_available_languages(){return
array('en'=>true,);}function
get_lang(){return
Locale::get()->getLanguage();}function
lang($t,$B=null){return
call_user_func_array([Locale::get(),"translate"],func_get_args());}function
get_language_options(){$Ra=get_available_languages();if(count($Ra)==1)return[];$C=[];foreach(Locale::$Languages
as$ag=>$T){if(isset($Ra[$ag]))$C[$ag]=$T;}return$C;}function
language_select(){$C=get_language_options();if(!$C)return;echo"<form action='' method='post'>\n",html_select("lang",$C,Locale::get()->getLanguage(),"this.form.submit();"),"<input type='submit' value='".lang(81),"' class='button hidden'>\n",input_token(),"</form>\n";}$Ra=get_available_languages();$ag=array_keys($Ra)[0];$Wi=null;if(isset($_POST["lang"])&&isset($Ra[$_POST["lang"]])&&verify_token()){$Wi=$_SESSION["lang"]=$_POST["lang"];$_SESSION["translations"]=[];}$Tj=($ra=Settings::readParameter("lang"))!==null?$ra:(isset($_COOKIE["neo_lang"])?$_COOKIE["neo_lang"]:null);if($Tj!==null&&isset($Ra[$Tj]))$ag=$Tj;elseif(isset($_SESSION["lang"])&&isset($Ra[$_SESSION["lang"]]))$ag=$_SESSION["lang"];elseif(isset($_SERVER["HTTP_ACCEPT_LANGUAGE"])){$ta=[];preg_match_all('~([-a-z]+)(;q=([0-9.]+))?~',str_replace("_","-",strtolower($_SERVER["HTTP_ACCEPT_LANGUAGE"])),$z,PREG_SET_ORDER);foreach($z
as$y)$ta[$y[1]]=(isset($y[3])?$y[3]:1);arsort($ta);foreach($ta
as$t=>$jj){if(isset($Ra[$t])){$ag=$t;break;}$t=preg_replace('~-.*~','',$t);if(!isset($ta[$t])&&isset($Ra[$t])){$ag=$t;break;}}}Locale::create($ag);abstract
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
openPasswordless($N,$V,$F,$Sk=true){$He=Admin::get()->getConfig()->getDefaultPasswordHash()!="";if($F!=""&&($Sk||$He)&&$this->open($N,$V,"")){$I=Admin::get()->verifyDefaultPassword($F);if($I!==true){$this->error=$I;return
false;}return
true;}return$this->open($N,$V,$F);}abstract
function
open($N,$V,$F);function
getFlavor(){return$this->flavor;}function
isMariaDB(){return$this->flavor=="mariadb";}function
isCockroachDB(){return$this->flavor=="cockroach";}function
getVersion(){return$this->version;}function
isMinVersion($Fm){return
version_compare($this->version,$Fm)>=0;}function
getAffectedRows(){return$this->affectedRows;}function
setAffectedRows($_a){$this->affectedRows=$_a;}function
getErrno(){return$this->errno;}function
getError(){return$this->error;}function
setError($i){$this->error=$i;}abstract
function
selectDatabase($A);abstract
function
quote($Tk);function
formatValue($Y,array$j){return$Y;}abstract
function
query($H,$gm=false);function
getQueryInfo(){return
null;}function
getResult($H,$j=0){return$this->getValue($H,$j);}function
getValue($H,$Nd=0){$I=$this->query($H);if(!is_object($I))return
false;$K=$I->fetchRow();return$K?$K[$Nd]:false;}function
multiQuery($H){$this->multiResult=$this->query($H);return(bool)($this->multiResult);}function
storeResult($I=null){return$this->multiResult;}function
nextResult(){return
false;}}abstract
class
Result{protected$rowsCount;function
__construct($Pj){$this->rowsCount=$Pj;}function
getRowsCount(){return$this->rowsCount;}abstract
function
fetchAssoc();abstract
function
fetchRow();abstract
function
fetchField();function
seek($Ch){return
false;}}if(extension_loaded('pdo')){abstract
class
PdoConnection
extends
Connection{protected$pdo;protected$multiResult;protected
function
dsn($Zc,$V,$F,array$C=[]){$C[PDO::ATTR_ERRMODE]=PDO::ERRMODE_SILENT;try{$this->pdo=new
PDO($Zc,$V,$F,$C);}catch(Exception$yd){$this->error=$yd->getMessage();return
false;}$this->version=preg_replace('~^\D*([\d.]+).*~',"$1",(string)@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION));return
true;}function
quote($Tk){return$this->pdo->quote($Tk);}function
query($H,$gm=false){$Qk=$this->pdo->query($H);$this->error="";if(!$Qk){list(,$this->errno,$this->error)=$this->pdo->errorInfo();if(!$this->error)$this->error=lang(122);return
false;}$I=new
PdoResult($Qk);$this->storeResult($I);return$I;}function
storeResult($I=null){if(!$I){$I=$this->multiResult;if(!$I)return
false;}if($I->getColumnsCount())return$I;$this->affectedRows=$I->getAffectedRowsCount();return
true;}function
nextResult(){return$this->multiResult&&$this->multiResult->nextRowset();}}class
PdoResult
extends
Result{private$statement;private$offset=0;function
__construct(PDOStatement$Qk){parent::__construct(max($Qk->columnCount()?$Qk->rowCount():0,0));$this->statement=$Qk;}function
getColumnsCount(){return$this->statement->columnCount();}function
getAffectedRowsCount(){return$this->statement->rowCount();}function
fetchAssoc(){return$this->fetchArray(PDO::FETCH_ASSOC);}function
fetchRow(){return$this->fetchArray(PDO::FETCH_NUM);}private
function
fetchArray($ah){$I=$this->statement->fetch($ah);return$I?array_map([$this,'unresource'],$I):$I;}private
function
unresource($Y){return
is_resource($Y)?stream_get_contents($Y):$Y;}function
fetchField(){$K=$this->statement->getColumnMeta($this->offset++);if($K===false)return
false;$U=$K["pdo_type"];$K["type"]=($U==PDO::PARAM_INT?0:15);$K["charsetnr"]=($U==\PDO::PARAM_LOB||(isset($K["flags"])&&in_array("blob",(array)$K["flags"]))?63:0);return(object)$K;}function
seek($Ch){for($p=0;$p<$Ch;$p++){if($this->statement->fetch()===false)return
false;;}return
true;}function
nextRowset(){$this->offset=0;return@$this->statement->nextRowset();}}}class
Drivers{private
static$drivers=[];private
static$extensions=[];static
function
add($q,$A,array$Gd){self::$drivers[$q]=$A;self::$extensions[$q]=$Gd;}static
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
setUserTypes(array$fm){$this->types[lang(109)]=array_flip($fm);}function
getUserTypes(){$t=lang(109);return
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
select($Q,array$M,array$Z,array$_e,array$D=[],$v=1,$E=0,$bj=false){$Bf=(count($_e)<count($M));$H="SELECT".limit(($_GET["page"]!="last"&&$v&&$_e&&$Bf&&DIALECT=="sql"?"SQL_CALC_FOUND_ROWS ":"").implode(", ",$M)."\nFROM ".table($Q),($Z?"\nWHERE ".implode(" AND ",$Z):"").($_e&&$Bf?"\nGROUP BY ".implode(", ",$_e):"").($D?"\nORDER BY ".implode(", ",$D):""),$v,($E?$v*$E:0),"\n");$Pk=microtime(true);$J=$this->connection->query($H);if($bj)echo
Admin::get()->formatSelectQuery($H,$Pk,!$J);return$J;}function
delete($Q,$mj,$v=0){$H="FROM ".table($Q);return
queries("DELETE".($v?limit1($Q,$H,$mj):" $H$mj"));}function
update($Q,array$rj,$mj,$v=0,$lk="\n"){$Cm=[];foreach($rj
as$t=>$X)$Cm[]="$t = $X";$H=table($Q)." SET$lk".implode(",$lk",$Cm);return
queries("UPDATE".($v?limit1($Q,$H,$mj,$lk):" $H$mj"));}function
insert($Q,array$rj){return
queries("INSERT INTO ".table($Q).($rj?" (".implode(", ",array_keys($rj)).")\nVALUES (".implode(", ",$rj).")":" DEFAULT VALUES").$this->getInsertReturningSql($Q));}function
getInsertReturningSql($Q){return"";}function
insertUpdate($Q,array$sj,array$aj){return
false;}function
begin(){return
queries("BEGIN");}function
commit(){return
queries("COMMIT");}function
rollback(){return
queries("ROLLBACK");}function
slowQuery($H,$Kl){return
null;}function
convertSearch($bf,array$Z,array$j){return$bf;}function
getNull(){return"NULL";}function
getTypeName(stdClass$j){return
isset($j->native_type)?$j->native_type:"";}function
quoteBinary($Tk){return
q($Tk);}function
warnings(){return
null;}function
tableHelp($A,$_f=false){return
null;}function
supportsIndex(array$ll){return!is_view($ll);}function
getIndexAlgorithms(array$ll){return[];}function
getIndexOpclasses(){return[];}function
getInheritedTables($Q){return[];}function
getParentTables($Q){return[];}function
isPartition($Q){return
false;}function
getPartitionsInfo($Q){return[];}function
hasCStyleEscapes(){return
false;}function
engines(){return[];}function
explodeArrayValue($Y,$U,&$Uj){return[];}function
implodeArrayValues(array$Cm,$U){return"";}function
checkConstraints($Q){return
get_key_vals("SELECT c.CONSTRAINT_NAME, CHECK_CLAUSE
FROM INFORMATION_SCHEMA.CHECK_CONSTRAINTS c
JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS t ON c.CONSTRAINT_SCHEMA = t.CONSTRAINT_SCHEMA
	AND c.CONSTRAINT_NAME = t.CONSTRAINT_NAME".($this->connection->isMariaDB()?" AND c.TABLE_NAME = ".q($Q):"")."
WHERE c.CONSTRAINT_SCHEMA = ".q($_GET["ns"]!=""?$_GET["ns"]:DB)."
AND t.TABLE_NAME = ".q($Q).(DIALECT=="pgsql"?"
AND CHECK_CLAUSE NOT LIKE '% IS NOT NULL'":""),$this->connection);}function
getAllFields(){if(DB=="")return[];$Ba=[];$Jf=(DIALECT=="pgsql"||DIALECT=="mssql");$aj=(DIALECT=="sql"?"c.COLUMN_KEY = 'PRI'":($Jf?"k.COLUMN_NAME":""));$L=get_rows("SELECT c.TABLE_NAME AS tab, c.COLUMN_NAME AS field, c.IS_NULLABLE AS nullable,
	c.DATA_TYPE AS type, c.CHARACTER_MAXIMUM_LENGTH AS length".($aj?",
	$aj AS ".idf_escape("primary"):"")."
FROM INFORMATION_SCHEMA.COLUMNS c".($Jf?"
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
open($N,$V,$F){mysqli_report(MYSQLI_REPORT_OFF);list($Ue,$Ri)=host_port($N);$t=Admin::get()->getConfig()->getSslKey();$lb=Admin::get()->getConfig()->getSslCertificate();$jb=Admin::get()->getConfig()->getSslCaCertificate();$Ok=$t||$lb||$jb;if($Ok){$this->mysqli->ssl_set($t,$lb,$jb,null,null);$be=Admin::get()->getConfig()->getSslTrustServerCertificate()?64:MYSQLI_CLIENT_SSL;}else$be=0;$Ub=@$this->mysqli->real_connect(($N!=""?$Ue:ini_get("mysqli.default_host")),($N.$V!=""?$V:ini_get("mysqli.default_user")),($N.$V.$F!=""?$F:ini_get("mysqli.default_pw")),null,(is_numeric($Ri)?(int)$Ri:ini_get("mysqli.default_port")),(!is_numeric($Ri)?$Ri:null),$be);$this->mysqli->options(MYSQLI_OPT_LOCAL_INFILE,false);if($Ub){$kf=$this->mysqli->get_server_info();$this->version=str_replace("-MariaDB","",$kf);$this->flavor=str_contains($kf,"MariaDB")?"mariadb":null;}return$Ub;}function
getAffectedRows(){return$this->mysqli->affected_rows;}function
getErrno(){return$this->mysqli->errno;}function
getError(){return$this->mysqli->error;}function
selectDatabase($A){return$this->mysqli->select_db($A);}function
setCharset($ob){if($this->mysqli->set_charset($ob))return
true;$this->mysqli->set_charset('utf8');return(bool)$this->query("SET NAMES $ob");}function
quote($Tk){return"'".$this->mysqli->escape_string($Tk)."'";}function
query($H,$gm=false){$I=$this->mysqli->query($H);return
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
__construct(mysqli_result$Fj){parent::__construct($Fj->num_rows);$this->resource=$Fj;}function
fetchAssoc(){return$this->resource->fetch_assoc();}function
fetchRow(){return$this->resource->fetch_row();}function
fetchField(){return$this->resource->fetch_field();}function
seek($Ch){return$this->resource->data_seek($Ch);}}}elseif(extension_loaded("pdo_mysql")){define("AdminNeo\DRIVER_EXTENSION","PDO_MySQL");class
MySqlConnection
extends
PdoConnection{function
getDefaultServerName(){return"localhost";}function
open($N,$V,$F){list($Ue,$Ri)=host_port($N);$Zc="mysql:charset=utf8".($Ue!=""?";host=$Ue":"").($Ri?(is_numeric($Ri)?";port=":";unix_socket=").$Ri:"");$C=[PDO::MYSQL_ATTR_LOCAL_INFILE=>false];$t=Admin::get()->getConfig()->getSslKey();if($t)$C[PDO::MYSQL_ATTR_SSL_KEY]=$t;$lb=Admin::get()->getConfig()->getSslCertificate();if($lb)$C[PDO::MYSQL_ATTR_SSL_CERT]=$lb;$jb=Admin::get()->getConfig()->getSslCaCertificate();if($jb)$C[PDO::MYSQL_ATTR_SSL_CA]=$jb;$bm=Admin::get()->getConfig()->getSslTrustServerCertificate();if($bm!==null&&defined('\PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT'))$C[PDO::MYSQL_ATTR_SSL_VERIFY_SERVER_CERT]=!$bm;if(!$this->dsn($Zc,$V,$F,$C))return
false;$Gm=@$this->pdo->getAttribute(PDO::ATTR_SERVER_VERSION);$this->flavor=str_contains($Gm,"MariaDB")?"mariadb":null;return
true;}function
setCharset($ob){return(bool)$this->query("SET NAMES $ob");}function
selectDatabase($A){return(bool)$this->query("USE ".idf_escape($A));}function
query($H,$gm=false){$this->pdo->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY,!$gm);return
parent::query($H,$gm);}}}class
MySqlDriver
extends
Driver{protected
function
__construct(Connection$d,$ya){parent::__construct($d,$ya);$this->types=[lang(123)=>["tinyint"=>3,"smallint"=>5,"mediumint"=>8,"int"=>10,"bigint"=>20,"decimal"=>66,"float"=>12,"double"=>21,],lang(124)=>["date"=>10,"datetime"=>19,"timestamp"=>19,"time"=>10,"year"=>4,],lang(125)=>["char"=>255,"varchar"=>65535,"tinytext"=>255,"text"=>65535,"mediumtext"=>16777215,"longtext"=>4294967295,],lang(126)=>["enum"=>65535,"set"=>64,],lang(127)=>["bit"=>20,"binary"=>255,"varbinary"=>65535,"tinyblob"=>255,"blob"=>65535,"mediumblob"=>16777215,"longblob"=>4294967295,],lang(128)=>["geometry"=>0,"point"=>0,"linestring"=>0,"polygon"=>0,"multipoint"=>0,"multilinestring"=>0,"multipolygon"=>0,"geometrycollection"=>0,],];$this->unsigned=["unsigned","zerofill","unsigned zerofill"];$_g=$d->isMariaDB();if($d->isMinVersion($_g?"10.2":"5.7"))$this->generated=["STORED","VIRTUAL"];$this->operators=["=","<",">","<=",">=","!=","LIKE","LIKE %%","NOT LIKE","IN","NOT IN","FIND_IN_SET","IS NULL","IS NOT NULL","REGEXP","NOT REGEXP","SQL",];$this->functions=["char_length","lower","upper","round","floor","ceil","date","from_unixtime","unix_timestamp","sec_to_time","time_to_sec",];$this->grouping=["sum","min","max","avg","count","count distinct","group_concat",];$this->partitionBy=["RANGE","LIST","HASH","LINEAR HASH","KEY","LINEAR KEY"];$this->insertFunctions=["char"=>"md5/sha1/password/encrypt/uuid","binary"=>"md5/sha1","date|time"=>"now",];$this->editFunctions=[number_type()=>"+/-","date"=>"+ interval/- interval","time"=>"addtime/subtime","char|text"=>"concat",];if($d->isMinVersion($_g?"10.2":"5.7.8"))$this->types[lang(125)]["json"]=4294967295;if($_g&&$d->isMinVersion("10.7")){$this->types[lang(125)]["uuid"]=128;$this->insertFunctions['uuid']='uuid';}if($_g&&$d->isMinVersion("10.5")){$this->types[lang(129)]["inet6"]=39;if($d->isMinVersion("10.10"))$this->types[lang(129)]["inet4"]=15;}if($d->isMinVersion($_g?"11.7":"9"))$this->types[lang(123)]["vector"]=16383;$this->systemDatabases=["mysql","information_schema","performance_schema","sys"];}function
insert($Q,array$rj){return($rj?parent::insert($Q,$rj):queries("INSERT INTO ".table($Q)." ()\nVALUES ()"));}function
getUnconvertFunction(array$j){if(preg_match("~binary~",$j["type"]))return"<code class='jush-sql'>UNHEX</code>";elseif($j["type"]=="bit")return
doc_link(['sql'=>'bit-value-literals.html','mariadb'=>"reference/sql-structure/sql-language-structure/binary-literals"],"<code>b''</code>");elseif($j["type"]=="vector")return"<code class='jush-sql'>".($this->connection->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."</code>";elseif(preg_match("~geometry|point|linestring|polygon~",$j["type"]))return"<code class='jush-sql'>GeomFromText</code>";else
return"";}function
getTypeName(stdClass$j){$fm=["decimal","tinyint","smallint","int","float","double",7=>"timestamp","bigint","mediumint","date","time","datetime","year",15=>"varchar","bit",242=>"vector",245=>"json","decimal","enum","set","tinytext","mediumtext","longtext","text","varchar","char","geometry",];$U=isset($fm[$j->type])?$fm[$j->type]:"";return
parent::getTypeName($j)?:($j->charsetnr==63?str_replace(["text","varchar","char"],["blob","varbinary","binary"],$U):$U);}function
quoteBinary($Tk){return"X".q(bin2hex($Tk));}function
insertUpdate($Q,array$sj,array$aj){$c=array_keys(reset($sj));$Xi="INSERT INTO ".table($Q)." (".implode(", ",$c).") VALUES\n";$Cm=[];foreach($c
as$t)$Cm[$t]="$t = VALUES($t)";$Zk="\nON DUPLICATE KEY UPDATE ".implode(", ",$Cm);$Cm=[];$u=0;foreach($sj
as$rj){$Y="(".implode(", ",$rj).")";if($Cm&&(strlen($Xi)+$u+strlen($Y)+strlen($Zk)>1e6)){if(!queries($Xi.implode(",\n",$Cm).$Zk))return
false;$Cm=[];$u=0;}$Cm[]=$Y;$u+=strlen($Y)+2;}return
queries($Xi.implode(",\n",$Cm).$Zk);}function
slowQuery($H,$Kl){$_g=$this->connection->isMariaDB();if(!$this->connection->isMinVersion($_g?"10.1.2":"5.7.8"))return
null;if($_g)return"SET STATEMENT max_statement_time=$Kl FOR $H";elseif(preg_match('~^(SELECT\b)(.+)~is',$H,$y))return"$y[1] /*+ MAX_EXECUTION_TIME(".($Kl*1000).") */ $y[2]";else
return
null;}function
convertSearch($bf,array$Z,array$j){return(preg_match('~char|text|enum|set~',$j["type"])&&!preg_match("~^utf8~",$j["collation"])&&preg_match('~[\x80-\xFF]~',$Z['val'])?"CONVERT($bf USING ".charset($this->connection).")":$bf);}function
warnings(){$I=$this->connection->query("SHOW WARNINGS");if($I&&$I->getRowsCount()){ob_start();print_select_result($I);return
ob_get_clean();}return
null;}function
tableHelp($A,$_f=false){$_g=$this->connection->isMariaDB();if(DB=="information_schema"){$A=strtolower($A);return$_g?"reference/system-tables/information-schema/information-schema-tables/".(str_starts_with($A,"innodb_")?"information-schema-innodb-tables/":"")."information-schema-$A-table":"information-schema-".str_replace("_","-",$A)."-table.html";}if(DB=="performance_schema")return$_g?"reference/system-tables/performance-schema/performance-schema-tables/performance-schema-$A-table":"performance-schema-".str_replace("_","-",$A)."-table.html";if(DB=="sys"){if($_g)return"reference/system-tables/sys-schema/";return"sys-".strtolower(str_replace("_","-",preg_replace('~^x\$~','',$A))).".html";}if(DB=="mysql")return$_g?"reference/system-tables/the-mysql-database-tables/mysql-$A".str_starts_with($A,"innodb_")?"":"-table":"system-schema.html";return
null;}function
getPartitionsInfo($Q){$pe="FROM information_schema.PARTITIONS WHERE TABLE_SCHEMA = ".q(DB)." AND TABLE_NAME = ".q($Q);$I=Connection::get()->query("SELECT PARTITION_METHOD, PARTITION_EXPRESSION, PARTITION_ORDINAL_POSITION $pe ORDER BY PARTITION_ORDINAL_POSITION DESC LIMIT 1")->fetchRow();if(!$I)return[];$kf=["partition_by"=>$I[0],"partition"=>$I[1],"partitions"=>$I[2],];$Bi=get_key_vals("SELECT PARTITION_NAME, PARTITION_DESCRIPTION $pe AND PARTITION_NAME != '' ORDER BY PARTITION_ORDINAL_POSITION");$kf["partition_names"]=array_keys($Bi);$kf["partition_values"]=array_values($Bi);return$kf;}function
getIndexAlgorithms(array$ll){return
preg_match('~^(MEMORY|NDB)$~',$ll["Engine"])?["BTREE","HASH"]:["BTREE"];}function
hasCStyleEscapes(){static$hb;if($hb===null){$Nk=$this->connection->getValue("SHOW VARIABLES LIKE 'sql_mode'",1);$hb=(strpos($Nk,'NO_BACKSLASH_ESCAPES')===false);}return$hb;}function
engines(){$pd=[];foreach(get_rows("SHOW ENGINES")as$K){if(preg_match("~YES|DEFAULT~",$K["Support"]))$pd[]=$K["Engine"];}return$pd;}}function
create_driver(Connection$d){return
MySqlDriver::create($d,Admin::get());}function
idf_escape($bf){return"`".str_replace("`","``",$bf)."`";}function
table($bf){return
idf_escape($bf);}function
connect($aj=false,&$i=null){$d=$aj?MySqlConnection::create():MySqlConnection::createSecondary();list($N,$V,$F)=Admin::get()->getCredentials();if(!$d->openPasswordless($N,$V,$F,false)){$i=$d->getError();if(function_exists('iconv')&&!is_utf8($i)&&strlen($Qj=iconv("windows-1252","utf-8//IGNORE",$i))>strlen($i))$i=$Qj;return
null;}$d->setCharset(charset($d));$d->query("SET sql_quote_show_create = 1, autocommit = 1");if($aj&&$d->isMariaDB()){Drivers::setName(DRIVER,"MariaDB");save_driver_name(DRIVER,$N,"MariaDB");}return$d;}function
get_databases($de){$f=get_session("dbs");if($f===null){$H="SELECT SCHEMA_NAME FROM information_schema.SCHEMATA ORDER BY SCHEMA_NAME";$Pk=microtime(true);$f=($de?slow_query($H):get_vals($H));if(microtime(true)-$Pk>0.1){restart_session();set_session("dbs",$f);stop_session();}}return$f;}function
limit($H,$Z,$v,$Ch=0,$lk=" "){return" $H$Z".($v?$lk."LIMIT $v".($Ch?" OFFSET $Ch":""):"");}function
limit1($Q,$H,$Z,$lk="\n"){return
limit($H,$Z,1,0,$lk);}function
db_collation($g,array$Cb){$J=null;$ec=Connection::get()->getValue("SHOW CREATE DATABASE ".idf_escape($g),1);if(preg_match('~ COLLATE ([^ ]+)~',$ec,$y))$J=$y[1];elseif(preg_match('~ CHARACTER SET ([^ ]+)~',$ec,$y))$J=$Cb[$y[1]][-1];return$J;}function
logged_user(){return
Connection::get()->getValue("SELECT USER()");}function
tables_list(){return
get_key_vals("SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() ORDER BY TABLE_NAME");}function
count_tables(array$f){$J=[];foreach($f
as$g)$J[$g]=count(get_vals("SHOW TABLES IN ".idf_escape($g)));return$J;}function
table_status($A="",$Ld=false){if($Ld)$H="SELECT TABLE_NAME AS Name, ENGINE AS Engine, CREATE_OPTIONS AS Create_options,
	TABLES.TABLE_COLLATION AS Collation, TABLE_COMMENT AS Comment
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() ".($A!=""?"AND TABLE_NAME = ".q($A):"ORDER BY Name");else$H="SHOW TABLE STATUS".($A!=""?" LIKE ".q(addcslashes($A,"%_\\")):"");$S=[];foreach(get_rows($H)as$K){if($K["Engine"]=="InnoDB")$K["Comment"]=preg_replace('~(?:(.+); )?InnoDB free: .*~','\1',$K["Comment"]);if(!isset($K["Engine"]))$K["Comment"]="";if($A!="")$K["Name"]=$A;$S[$K["Name"]]=$K;}return$S;}function
is_view(array$R){return$R["Engine"]===null;}function
fk_support(array$R){return
preg_match('~InnoDB|IBMDB2I'.(Connection::get()->isMinVersion("5.6")?'|NDB':'').'~i',$R["Engine"]);}function
fields($Q){$_g=Connection::get()->isMariaDB();$J=[];foreach(get_rows("SELECT * FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ".q($Q)." ORDER BY ORDINAL_POSITION")as$K){$j=$K["COLUMN_NAME"];$U=preg_replace('~\s?/\*.+\*/~U',"",$K["COLUMN_TYPE"]);$Hd=$K["EXTRA"];preg_match('~^(VIRTUAL|PERSISTENT|STORED)~',$Hd,$te);preg_match('~^([^( ]+)(?:\((.+)\))?( unsigned)?( zerofill)?$~',$U,$em);$h=$_g&&$K["COLUMN_DEFAULT"]=="NULL"?null:$K["COLUMN_DEFAULT"];if($h!==null){$Ef=preg_match('~(text|json)~',$em[1]);if(!$_g&&$Ef)$h=preg_replace("~^(_\w+)?('.*')$~",'\2',stripslashes($h));if($_g||$Ef){$h=preg_replace_callback("~^'(.*)'$~",function($z){return
stripslashes(str_replace("''","'",$z[1]));},$h);}if(!$_g&&preg_match('~binary~',$em[1])&&preg_match('~^0x(\w*)$~',$h,$z))$h=pack("H*",$z[1]);}$ve=$K["GENERATION_EXPRESSION"];if(!$_g)$ve=preg_replace("~(^|,|\()(_\w+)?('.*')($|,|\))~",'\1\3\4',stripslashes($ve));$J[$j]=["field"=>$j,"full_type"=>$U,"type"=>$em[1],"length"=>$em[2],"unsigned"=>ltrim($em[3].$em[4]),"default"=>($te?$ve:$h),"null"=>($K["IS_NULLABLE"]=="YES"),"auto_increment"=>($Hd=="auto_increment"),"on_update"=>(preg_match('~\bon update (\w+)~i',$Hd,$em)?$em[1]:""),"collation"=>$K["COLLATION_NAME"],"privileges"=>array_flip(explode(",",$K["PRIVILEGES"]))+["where"=>1,"order"=>1],"comment"=>$K["COLUMN_COMMENT"],"primary"=>($K["COLUMN_KEY"]=="PRI"),"generated"=>($te[1]=="PERSISTENT"?"STORED":$te[1]),];}return$J;}function
indexes($Q,$d=null){$J=[];foreach(get_rows("SHOW INDEX FROM ".table($Q),$d)as$K){$A=$K["Key_name"];$J[$A]["type"]=($A=="PRIMARY"?"PRIMARY":($K["Index_type"]=="FULLTEXT"?"FULLTEXT":($K["Non_unique"]?(preg_match('~^(SPATIAL|VECTOR)$~',$K["Index_type"])?$K["Index_type"]:"INDEX"):"UNIQUE")));$J[$A]["columns"][]=$K["Column_name"];$J[$A]["lengths"][]=($K["Index_type"]=="SPATIAL"?null:$K["Sub_part"]);$J[$A]["descs"][]=($K["Collation"]=="D"?'1':null);$J[$A]["algorithm"]=$K["Index_type"];}return$J;}function
foreign_keys($Q){static$Hi='(?:`(?:[^`]|``)+`|"(?:[^"]|"")+")';$J=[];$gc=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if($gc){$Mh=implode("|",Driver::get()->getOnActions());preg_match_all("~CONSTRAINT ($Hi) FOREIGN KEY ?\\(((?:$Hi,? ?)+)\\) REFERENCES ($Hi)(?:\\.($Hi))? "."\\(((?:$Hi,? ?)+)\\)(?: ON DELETE ($Mh))?(?: ON UPDATE ($Mh))?~",$gc,$z,PREG_SET_ORDER);foreach($z
as$y){preg_match_all("~$Hi~",$y[2],$Hk);preg_match_all("~$Hi~",$y[5],$_l);$J[idf_unescape($y[1])]=["db"=>idf_unescape($y[4]!=""?$y[3]:$y[4]),"table"=>idf_unescape($y[4]!=""?$y[4]:$y[3]),"source"=>array_map('AdminNeo\idf_unescape',$Hk[0]),"target"=>array_map('AdminNeo\idf_unescape',$_l[0]),"on_delete"=>($y[6]?:"RESTRICT"),"on_update"=>($y[7]?:"RESTRICT"),];}}return$J;}function
backward_keys($Q){$H="SELECT CONSTRAINT_NAME AS constraint_name, TABLE_SCHEMA AS table_schema, TABLE_NAME AS table_name,
COLUMN_NAME AS column_name, REFERENCED_COLUMN_NAME AS referenced_column_name
FROM information_schema.KEY_COLUMN_USAGE
WHERE TABLE_SCHEMA = ".q(Admin::get()->getDatabase())."
AND REFERENCED_TABLE_SCHEMA = ".q(Admin::get()->getDatabase())."
AND REFERENCED_TABLE_NAME = ".q($Q)."
ORDER BY ORDINAL_POSITION";return
get_rows($H,null,"");}function
view($A){$M=Connection::get()->getValue("SHOW CREATE VIEW ".table($A),1);$xg='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$M=preg_replace("~^$xg\\s+AS\\s+~isU","",$M);return["select"=>format_sql($M)];}function
collations(){$J=[];$H=Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("10.10")?"SELECT CHARACTER_SET_NAME AS Charset, FULL_COLLATION_NAME AS Collation, IS_DEFAULT AS `Default` FROM information_schema.COLLATION_CHARACTER_SET_APPLICABILITY":"SHOW COLLATION";foreach(get_rows($H)as$K){if($K["Default"])$J[$K["Charset"]][-1]=$K["Collation"];else$J[$K["Charset"]][]=$K["Collation"];}ksort($J);foreach($J
as$t=>$X)sort($J[$t]);return$J;}function
information_schema($g,$Yj=""){return($g=="information_schema")||(Connection::get()->isMinVersion("5.5")&&$g=="performance_schema");}function
error(){return
h(preg_replace('~^You have an error.*syntax to use~U',"Syntax error",Connection::get()->getError()));}function
create_database($g,$Bb){return(bool)queries("CREATE DATABASE ".idf_escape($g).($Bb?" COLLATE ".q($Bb):""));}function
drop_databases(array$f){$J=apply_queries("DROP DATABASE",$f,'AdminNeo\idf_escape');restart_session();set_session("dbs",null);return$J;}function
rename_database($A,$Bb){$J=false;if(create_database($A,$Bb)){$S=[];$Jm=[];foreach(tables_list()as$Q=>$U){if($U=='VIEW')$Jm[]=$Q;else$S[]=$Q;}$J=(!$S&&!$Jm)||move_tables($S,$Jm,$A);drop_databases($J?[DB]:[]);}return$J;}function
auto_increment(){$Pa=" PRIMARY KEY";if($_GET["create"]!=""&&$_POST["auto_increment_col"]){foreach(indexes($_GET["create"])as$r){if(in_array($_POST["fields"][$_POST["auto_increment_col"]]["orig"],$r["columns"],true)){$Pa="";break;}if($r["type"]=="PRIMARY")$Pa=" UNIQUE";}}return" AUTO_INCREMENT$Pa";}function
alter_table($Q,$A,array$k,array$fe,$Lb,$od,$Bb,$Oa,$Ai){$Ga=[];foreach($k
as$j){if($j[1]){$h=$j[1][3];if(str_contains($h," GENERATED")){$j[1][3]=Connection::get()->isMariaDB()?"":$j[1][2];$j[1][2]=$h;}$Ga[]=($Q!=""?($j[0]!=""?"CHANGE ".idf_escape($j[0]):"ADD"):" ")." ".implode($j[1]).($Q!=""?$j[2]:"");}else$Ga[]="DROP ".idf_escape($j[0]);}$Ga=array_merge($Ga,$fe);$P=($Lb!==null?" COMMENT=".q($Lb):"").($od?" ENGINE=".q($od):"").($Bb?" COLLATE ".q($Bb):"").($Oa!=""?" AUTO_INCREMENT=$Oa":"");if($Ai){$Bi=[];if($Ai["partition_by"]=='RANGE'||$Ai["partition_by"]=='LIST'){foreach($Ai["partition_names"]as$t=>$X){$Y=$Ai["partition_values"][$t];$Bi[]="\n  PARTITION ".idf_escape($X)." VALUES ".($Ai["partition_by"]=='RANGE'?"LESS THAN":"IN").($Y!=""?" ($Y)":" MAXVALUE");}}$P
.="\nPARTITION BY {$Ai["partition_by"]}({$Ai["partition"]})";if($Bi)$P
.=" (".implode(",",$Bi)."\n)";elseif($Ai["partitions"])$P
.=" PARTITIONS ".(int)$Ai["partitions"];}elseif($Ai===null)$P
.="\nREMOVE PARTITIONING";if($Q=="")return(bool)queries("CREATE TABLE ".table($A)." (\n".implode(",\n",$Ga)."\n)$P");if($Q!=$A)$Ga[]="RENAME TO ".table($A);if($P)$Ga[]=ltrim($P);return!$Ga||queries("ALTER TABLE ".table($Q)."\n".implode(",\n",$Ga));}function
alter_indexes($Q,array$Ga){$nb=[];foreach($Ga
as$t=>$X)$nb[]=($X[2]=="DROP"?"\nDROP INDEX ".idf_escape($X[1]):"\nADD $X[0] ".($X[0]=="PRIMARY"?"KEY ":"").($X[1]!=""?idf_escape($X[1])." ":"")."(".implode(", ",$X[2]).")");return(bool)queries("ALTER TABLE ".table($Q).implode(",",$nb));}function
truncate_tables(array$S){return
apply_queries("TRUNCATE TABLE",$S);}function
drop_views(array$Jm){return(bool)queries("DROP VIEW ".implode(", ",array_map('AdminNeo\table',$Jm)));}function
drop_tables(array$S){return(bool)queries("DROP TABLE ".implode(", ",array_map('AdminNeo\table',$S)));}function
move_tables(array$S,array$Jm,$_l){$Cj=[];foreach($S
as$Q)$Cj[]=table($Q)." TO ".idf_escape($_l).".".table($Q);if(!$Cj||queries("RENAME TABLE ".implode(", ",$Cj))){$_c=[];foreach($Jm
as$Q)$_c[table($Q)]=view($Q);Connection::get()->selectDatabase($_l);$g=idf_escape(DB);foreach($_c
as$A=>$Hm){if(!queries("CREATE VIEW $A AS ".str_replace(" $g."," ",$Hm["select"]))||!queries("DROP VIEW $g.$A"))return
false;}return
true;}return
false;}function
copy_tables(array$S,array$Jm,$_l){queries("SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO'");foreach($S
as$Q){$A=($_l==DB?table("copy_$Q"):idf_escape($_l).".".table($Q));if(($_POST["overwrite"]&&!queries("\nDROP TABLE IF EXISTS $A"))||!queries("CREATE TABLE $A LIKE ".table($Q))||!queries("INSERT INTO $A SELECT * FROM ".table($Q)))return
false;foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")))as$K){$Yl=$K["Trigger"];if(!queries("CREATE TRIGGER ".($_l==DB?idf_escape("copy_$Yl"):idf_escape($_l).".".idf_escape($Yl))." $K[Timing] $K[Event] ON $A FOR EACH ROW\n$K[Statement];"))return
false;}}foreach($Jm
as$Q){$A=($_l==DB?table("copy_$Q"):idf_escape($_l).".".table($Q));$Hm=view($Q);if(($_POST["overwrite"]&&!queries("DROP VIEW IF EXISTS $A"))||!queries("CREATE VIEW $A AS $Hm[select]"))return
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
routine_id($A,array$K){return
idf_escape($A);}function
last_id($I){return
Connection::get()->getValue("SELECT LAST_INSERT_ID()");}function
explain(Connection$d,$H){return$d->query("EXPLAIN ".(Connection::get()->isMinVersion("5.7")?"":"PARTITIONS ").$H);}function
found_rows(array$R,array$Z){return$R["Engine"]=="InnoDB"&&!$Z?(int)$R["Rows"]:null;}function
format_sql($H){$xg='(?:[^`\']|`[^`]*`|\'[^\']*\')*';$Rf='FROM|WHERE|HAVING|GROUP\s+BY|ORDER\s+BY|(NATURAL\s+)?((LEFT|RIGHT)\s+)?((INNER|OUTER|CROSS)\s+)?JOIN';$H=preg_replace("~($xg)\\s+(AS\\s+SELECT)~isU","$1 AS\nSELECT",$H);$H=preg_replace("~($xg)\\s+($Rf)~isU","$1\n$2",$H);$H=preg_replace("~($xg),~isU","$1,\n  ",$H);return$H;}function
create_sql($Q,$Oa,$Wk){$H=Connection::get()->getValue("SHOW CREATE TABLE ".table($Q),1);if(!$Oa)$H=preg_replace('~ AUTO_INCREMENT=\d+~','',$H);return!str_contains($H,"\n")?format_sql($H):$H;}function
truncate_sql($Q){return"TRUNCATE ".table($Q);}function
create_database_sql($qc,$Wk=""){$A=idf_escape($qc);$Jb="";if(str_contains($Wk,"CREATE")&&($ec=Connection::get()->getValue("SHOW CREATE DATABASE $A",1))){set_utf8mb4($ec);if($Wk=="DROP+CREATE")$Jb="DROP DATABASE IF EXISTS $A;\n";$Jb
.="$ec;\n";}return$Jb;}function
use_sql($qc,$Wk=""){return"USE ".idf_escape($qc).";\n";}function
trigger_sql($Q){$Kk="";foreach(get_rows("SHOW TRIGGERS LIKE ".q(addcslashes($Q,"%_\\")),null,"-- ")as$K)$Kk
.="\nCREATE TRIGGER ".idf_escape($K["Trigger"])." $K[Timing] $K[Event] ON ".table($K["Table"])." FOR EACH ROW\n$K[Statement];;\n";return$Kk;}function
show_variables(){return
get_rows("SHOW VARIABLES");}function
show_status(){return
get_rows("SHOW STATUS");}function
process_list(){return
get_rows("SHOW FULL PROCESSLIST");}function
convert_field(array$j){if(preg_match("~binary~",$j["type"]))return"HEX(".idf_escape($j["field"]).")";if($j["type"]=="bit")return"BIN(".idf_escape($j["field"])." + 0)";if($j["type"]=="vector")return(Connection::get()->isMariaDB()?"VEC_ToText":"VECTOR_TO_STRING")."(".idf_escape($j["field"]).")";if(preg_match("~geometry|point|linestring|polygon~",$j["type"]))return(Connection::get()->isMinVersion("8")?"ST_":"")."AsWKT(".idf_escape($j["field"]).")";return
null;}function
unconvert_field(array$j,$J){if(preg_match("~binary~",$j["type"]))$J="UNHEX($J)";if($j["type"]=="bit")$J="CONVERT(b$J, UNSIGNED)";if($j["type"]=="vector")$J=(Connection::get()->isMariaDB()?"VEC_FromText":"STRING_TO_VECTOR")."($J)";if(preg_match("~geometry|point|linestring|polygon~",$j["type"])){$Xi=(Connection::get()->isMinVersion("8")?"ST_":"");$J=$Xi."GeomFromText($J, $Xi"."SRID($j[field]))";}return$J;}function
support($Md){return
preg_match('~^(comment|columns|copy|database|drop_col|dump|event|indexes|kill|privileges|move_col|procedure|processlist|routine|sql|status|table|trigger|variables|view'.(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.8.1":"8")?'|descidx':'').(Connection::get()->isMinVersion(Connection::get()->isMariaDB()?"10.2.1":"8.0.16")?'|check':'').(!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8")?'|fast_status':'').')$~',$Md);}function
kill_process($X){return
queries("KILL ".number($X));}function
connection_id(){return"SELECT CONNECTION_ID()";}function
max_connections(){return(int)Connection::get()->getValue("SELECT @@max_connections");}}$Pi="adminneo-plugins";if(is_dir($Pi)){foreach(glob("$Pi/*.php")as$m)include_once$m;}function
get_translations($Zf){switch($Zf){case'en':$Qb='+X/+JaMAp,z0
Y+"&I/g5m.fI,3%v3SjRAY<XSXdB=b??hR,(W^O`Dah[mx^O2qSz5e
%m^f:C6Dt5>&aKB69V(!du96/vM_*
<%SMQF=N<O$lz70RynoNp6._7`CFqcYK1jGxNF(Rg5%/KKgvic"&W=+qRf>jg,^bX]l_OiwUq/N#B=][3f!EYYh[MZwvPr,yu9,JDT
H+1A
mjf`r1VUXOQJTc/A
Z$#{MfHRr$
ZB1]~xayv?hEQ*|na*ayTP(MnaZMtidxPv_y^59`[jU>Blyq@aO@Y69OEdFyi;sP%4;GG<dF)n1,C3RZ$w0F541@y*4P$mUbh/`mU^2:=7D#
1!W{1VbeXn+X#N@xuDVXbJsp:!__^-IC9(@RKr==1Dbk?j[/8UCq2gpW=%gf)?7}8r7yheU4@X3j8_mZ6C9nAzfl:u&#OO&cGNR[C&UugGV|1V(}(pk)n^oq)O_iob[/xgQlZ(p>1DS3&oRG+tjZpEEEk^bnY4f?f:+?
*l&LR9AD!@V+5LgKs+<0o.%HQwboH#F.BgHl*gK"}Bnh-3U/xkg60#:qYg5;DcRKHqkMA?hk*Pq7Y>vH32b.qZgQZpy1j7DtoBCj^G^6
y|t_"FDyck5J
n9{lQ)tspby^s34dNO,o%v)Ov,vi|Bg?`S>NyxVnXq]xPC)NU([GG``0C5x+d].PSVbJ|c;fbM">;^Zfl!XlSxPk@_[)kG"h*Wiajh+,geuZ(uPD|3DU4<6?<iH]VuhKRN^?/idl-E7WFKv6n>uFpDn#pHM$kH-9]x|E&*5a{
.H@B@FFVF-`/<k@qm^GWO2a:^r;X!OLqt+I46r~+;lXZ}t9w.Oet&XrLlic=/fWR)&GDXcvLLc*cj($X-a.STIrH,uq?fJ5xxCl+rV"V$O{9_E0d|]0H,>,a7A!G2/59B?mFBIsC!%d`eExDg)L"g7]_[`#1i"dXl.lR50<qT83GMx~)p0VrLvJPM[BZbf(bL^5B5TsG~[<i.uliDYKBLHdty%LkbyhB`6{=nNz`V?ZjkT#]Dc/r{kLf{Gor^xPQhZF8!]rY?o:m}sMqH>v:TVu<h2nrw17[eq3LQ6}G[O}?LbpS&>mZ!p:Y[CQ]7+.DzGrZOV=Z01QUj5hW1Bl+J3.qGWV>]xcw@:gw;p?z%uFl?wvp#Q>-H,aqo,Kih8/q6HFH/.f<LLv"*=ZdWHl-]ynq0b$]?HI>|lRV2]Z5<AG3Y&YIX!@sUeel@91-PI-gR*J3vpm_#@$fAiNXsy}G)iZ*RAI&B<YM+I??*1O+jWS1hNA$pS]-0`pnP$E$Z>"sk8
<zm#fyZdvQc?:hCJ9.#By7<sc[-9d!2ftFmuEzd<%C-A5p,e-Kler+/4(+0HOjaGMs/ha9)DNQXTw9eXqKbhNK)X?;jg<f<(hBN%OgZxVO8D[)f.+kdeU0.Vc:N4JVd8;4BQUcJvhC5maI(5s)Z?;AwfldGM26HCW]TRKU21<9=8.&4<JF8~^J;6A1/1fNk6/PVQrXkyx>:}a)a51M@vJ#cWLl?G>zBSCn=|D]GqQ^?+x"e*2s5^qO[n3fUovGgo>Tvt!&J9Q3%u_oA318Y.n/9x=`/dgDCO]
I#I|M37AN?qf$q[-s
8(,k.a;@DzWVcWMp23EEVB^r$"*Re%rTAwDy+NHD(~9x6
%#M/*%izi}h03+-+W|X"kht?*gC<770Vl=vWef-ccW/^G-J}^=4kj!GgCec$PMVwsR7tV?%*)_5d:C/lU_d
X?1B$A
V3_D":E0nYk]S<o?9Ar*;9Lr(Nl!|Dyf&`%JovXcw@Qv"ho3#]1.Ih6C#yq>VeP
WeWp"/IlV.$mH[("1hC>g-Cwm@fuXZ*rpF
E>0u"r"a*}Oe6=4T;
^FDf,Joxb+$a8:K=YqDD)#k$#Lf;n*4L%bh2azMS:mPvg>&<HPXs*P9UKj#xFf!jP%9K28-.;=0s!ZKk@]#
!U.3EufMZ^xWP`$1u%3>L*o1:ID<Z`A4s`hjL2
c.mE[7?tNZ&,.Qog>_Z+#d~D(.G68peQ`%%@gx8mGYaZ~,>On0/6uC5HT.9o,lU*ZZp2sF(njElVeNMbPb4X[Qv;]yzc88==W2rxlk)+b=8:&V8YF,RJ_N`!j*pd}yFMX[8Wd#..$9Wte=":1m+e!FC>q_4x@3K;eQ6]5M+JGwy
Yd@w93[DdXeHFKUE&d!F=uMD
0,xN9e$mMNnV[_2;h?#V<kB~8mHh#29<31dKJAE16ZhPko<Dvol<p?7UKf1)m@ftKwsgU.wo>
<clqqtSeiJf>]W38!3f6T%q6R>:{6]dc#1/[%{+eu@8q$3P$/Zwo4Ln8GS0{8]/"F_jI4C[1NqU)E:&ir8YGk
un6}T6<P&@g(f#>rF5GWA/8:X.D}A^V[$=%(boP>]rr?Wx9C2B`~vOiith=<ng<L/x5dL]1t/JDH)$V8C@Y%7YcB(sIM:KqG8c/|&owmXf%9CL5lJ^6Eo8)G@rb.pnm
]#%8KeST0P0)LH-/0FRYji07:^uK&8TFDe<pk%-0k!ThJ1&
K[jT9pX;q*99j|]av^Wy,!nxap2m?ROz:RlYHfR6tvB.%GiCacua^le>:Ae6BO7sQL9iB6crbb9J"lcIJ7-~31rL,p.<$L?;Q/f+S^n[v+ctXxB~7G2#j$F4a2FXKhCATgJ4&e8r
_wy!g2?Y,iE;A^Dk1c?1qao!9LJww#k^MDMi3DMw+kUyzsIw9i&GWy5C;AEao@i3k63u]X6>s#]?3K4/0];/VhzJM0X4MXQTD2:k*j}Z6a?&]bGfj/Qy_)t/G7W&8tb1l60RD1Y1JN-Y;WJm0Yf0?>"Aobe>o0xDvC|v|0"Thw_"#%P&qnCaTbAp/;Nq,C~
*@TU0X^T!Nw=a6/QY,tq!>oqhXE<_%
arVj.l;C7t>7xU&Uo/d;R`RN$fGxTc"qsNBwyPRE6.jX#z]D4C!hgka]rV%Ji,m.de,r
L$<lu3v<Dneu61D2GW@!n_XfBq6LQL$x`q|v#wj`?iE<yxIw<2]54R^8W%;
=ZtYWah
IVJWfV6-E
HbqTv03%H#pOX8rb_Bnl_`W;@I0UDulb=,xa7r(Wx3ZmE]Qa{.C7;3d]6tl*4F]M;sn)Wq?grsT4uk35D1].sg*0[+>cu!Q';break;}return
json_decode(decompress_string($Qb),true);}function
get_plural_translation_id($t){$Qi=array('Too many unsuccessful logins, try again in %d minute(s).'=>141,'%d process(es) have been killed.'=>279,'%d query(s) executed OK.'=>197,'Query executed OK, %d row(s) affected.'=>195,'%d row(s) have been imported.'=>286,'Routine has been called, %d row(s) affected.'=>231,'%d row(s)'=>194,'%d byte(s)'=>43,'%d item(s) have been affected.'=>283,);return
isset($Qi[$t])?$Qi[$t]:null;}$Xl=$_SESSION["translations"];$ag=Locale::get()->getLanguage();if($_SESSION["translations_version"]!=3879775018){$Xl=[];$_SESSION["translations_version"]=3879775018;}if($_SESSION["translations_language"]!=$ag){$Xl=[];$_SESSION["translations_language"]=$ag;}if(!$Xl){$Xl=get_translations($ag);$_SESSION["translations"]=$Xl;}Locale::get()->setTranslations($Xl);$ya=null;$lc=false;$uf=null;if(function_exists('\adminneo_instance')){$ya=\adminneo_instance();$lc=true;}elseif(file_exists("adminneo-instance.php")){$ya=include_once"adminneo-instance.php";$lc=true;}if($lc&&!$ya
instanceof
Admin&&!$ya
instanceof
Pluginer){$ya=null;$qg="href=https://github.com/adminneo-org/adminneo#advanced-customizations ".target_blank();$uf=lang(130,"<b>adminneo-instance.php</b>","<b>adminneo_instance()</b>","Admin::create()")." <a $qg>".lang(1)."</a>";}if(!$ya)$ya=Admin::create();if($uf)$ya->addError($uf);if($Wi!==null&&!isset($_GET["settings"])){$ya->getSettings()->updateParameter("lang",$Wi);redirect(remove_from_uri());}if(!defined("AdminNeo\DRIVER")){define("AdminNeo\DRIVER",null);define("AdminNeo\DIALECT",null);}define("AdminNeo\SERVER",DRIVER?$_GET[DRIVER]:null);define("AdminNeo\DB",isset($_GET["db"])?$_GET["db"]:"");define("AdminNeo\BASE_URL",preg_replace('~\?.*~','',relative_uri()));define("AdminNeo\ME",BASE_URL.'?'.(sid()?session_name()."=".urlencode(session_id()).'&':'').(SERVER!==null?DRIVER."=".urlencode(SERVER).'&':'').($_GET["ext"]?"ext=".urlencode($_GET["ext"]).'&':'').(isset($_GET["username"])?"username=".urlencode($_GET["username"]).'&':'').(DB!=""?'db='.urlencode(DB).'&'.(isset($_GET["ns"])?"ns=".urlencode($_GET["ns"])."&":""):''));define("AdminNeo\HOME_URL",BASE_URL?:".");define("AdminNeo\SERVER_HOME_URL",substr(preg_replace('~\b(username|db|ns)=[^&]*&~','',ME),0,-1)?:".");if(isset($_GET["set"])){header("Content-Type: text/javascript; charset=utf-8");if(!verify_token()){header("HTTP/1.1 403 Forbidden");exit;}if($_GET["set"]=="navigation-width"){$Wm=isset($_POST["width"])?$_POST["width"]:"";if($Wm!=""){$Wm=min(max((float)$Wm,Settings::$NavigationWidthMin),Settings::$NavigationWidthMax);Admin::get()->getSettings()->updateParameter("navigationWidth",sprintf("%.2F",$Wm));}else
Admin::get()->getSettings()->updateParameter("navigationWidth",null);}if($_GET["set"]=="export-settings")Admin::get()->getSettings()->updateParameters(["exportFormat"=>isset($_POST["format"])?$_POST["format"]:"","exportOutput"=>isset($_POST["output"])?$_POST["output"]:"",]);exit;}const
VERSION="5.8.0";function
page_header($T,$db=[]){if(!headers_sent()&&!array_sum(array_column(ob_get_status(true),"buffer_used")))ini_set("zlib.output_compression","1");page_headers();if(is_ajax()&&Admin::get()->getErrors()){page_messages();exit;}if(!ob_get_level())ob_start(null,4096);$T=strip_tags($T);$sk=$db!==false&&$db!==null&&SERVER!=""?" - ".h(Admin::get()->getServerName(SERVER)):"";$uk=strip_tags(Admin::get()->getServiceTitle());$Nl=$T.$sk." - ".($uk!=""?$uk:"AdminNeo");echo'<!DOCTYPE html>
<html lang=\'',Locale::get()->getLanguage(),'\' dir=\'',lang(131),'\' class=\'',lang(131),' nojs\'>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<meta name="robots" content="noindex, nofollow">
	<meta name="viewport" content="width=device-width, initial-scale=1"/>

	<title>',$Nl,'</title>

	';$Eb=validate_color_variant(Admin::get()->getConfig()->getColorVariant());echo"<link rel='stylesheet' href='",link_files("default-$Eb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("default-$Eb-dark.css",[]),"'>\n";$Gl=Admin::get()->getSettings()->getTheme();list($Gl,$Eb)=validate_theme($Gl,$Eb);if($Gl!="default"){echo"<link rel='stylesheet' href='",link_files("$Gl-$Eb.css",[]),"'>\n";if(!Admin::get()->isLightModeForced())echo"<link rel='stylesheet' ".(!Admin::get()->isDarkModeForced()?"media='(prefers-color-scheme: dark)' ":"")."href='",link_files("$Gl-$Eb-dark.css",[]),"'>\n";}foreach(Admin::get()->getCssUrls()as$rm){if(strpos($rm,"adminneo-dark.css")===0&&!Admin::get()->isDarkModeForced())echo"<link rel='stylesheet' media='(prefers-color-scheme: dark)' href='",h($rm),"'>\n";else
echo"<link rel='stylesheet' href='",h($rm),"'>\n";}$oh=Admin::get()->getSettings()->getNavigationWidth();echo"<style id='navigation-width'>";if($oh)echo"@media screen and (min-width: 1024px) { :root { --menu-width: ",sprintf("%.2F",$oh),"rem } }";echo"</style>\n",script_src(link_files("main.js",[]));foreach(Admin::get()->getJsUrls()as$rm)echo
script_src($rm);Admin::get()->printFavicons();Admin::get()->printToHead();echo'</head>
<body>
<script',nonce(),'>
	// The event handlers and the <html> classes are registered by functions.js.
	const offlineMessage = \'',js_escape(lang(132)),'\';
	const thousandsSeparator = \'',js_escape(lang(107)),'\';
</script>


',"<div id='help' class='jush-".DIALECT." jsonly hidden'></div>",script("initHelpPopup();"),'<menu class="access-menu">','<li><a href="#main-content">'.lang(133).'</a></li>','<li><a class="panel-link" href="#navigation-panel">'.lang(134).'</a></li>';if($db!==null&&DB!=""&&$_GET["ns"]!=="")echo'<li><a class="panel-link" href="#tables">'.lang(135).'</a></li>';echo'</menu>',"<div id='content'>\n","<div class='header'>\n","<button id='open-navigation-button' type='button' class='button light navigation-button' title='",lang(136),"' aria-controls='navigation-panel' aria-expanded='false'>",icon_solo("menu"),"</button>";if($db!==null){echo'<nav class="breadcrumbs"><ul>',"<li><a href='".h(HOME_URL)."' title='",lang(137),"'>",icon_solo("home"),"</a></li>";$qk=h(Admin::get()->getServerName(SERVER??""));if($db===false)echo"<li>$qk</li>";else{$w=substr(preg_replace('~\b(db|ns)=[^&]*&~','',ME),0,-1);echo"<li><a href='".h($w)."' accesskey='1' title='Alt+Shift+1'>$qk</a></li>";if($_GET["ns"]!=""||(DB!=""&&is_array($db)))echo'<li><a href="'.h($w."&db=".urlencode(DB).(support("scheme")?"&ns=":"")).'">'.h(DB).'</a></li>';if($db===true){if($_GET["ns"]!="")echo'<li>'.h($_GET["ns"]).'</li>';else
echo"<li>",h(DB),"</li>";}else{if($_GET["ns"]!="")echo'<li><a href="'.h(substr(ME,0,-1)).'">'.h($_GET["ns"]).'</a></li>';foreach($db
as$t=>$X){if(is_string($t)){$Cc=(is_array($X)?$X[1]:h($X));if($Cc!="")echo"<li><a href='".h(ME."$t=").urlencode(is_array($X)?$X[0]:$X)."'>$Cc</a></li>";}else
echo"<li>$X</li>\n";}}}echo"</ul></nav>";}echo"</div>\n","<div id='main-content'>\n","<h1>$T</h1>\n","<div id='ajaxstatus' role='status' class='jsonly'></div>\n";restart_session();page_messages();$f=&get_session("dbs");if(DB!=""&&$f&&!in_array(DB,$f,true))$f=null;stop_session();define("AdminNeo\PAGE_HEADER",1);ob_flush();flush();}function
validate_color_variant($Eb){list(,$Eb)=validate_theme("default",$Eb);return$Eb;}function
validate_theme($Gl,$Eb){$Hl=get_available_themes();if(isset($Hl[$Gl][$Eb]))return[$Gl,$Eb];if(isset($Hl["default"][$Eb]))return["default",$Eb];reset($Hl["default"]);return["default",key($Hl["default"])];}function
get_available_themes(){return
array('default'=>array('blue'=>true,),);}function
get_theme_titles($Eb){$Hl=get_available_themes();$Ol=[];foreach($Hl
as$Gl=>$Fb){if(isset($Fb[$Eb])?$Fb[$Eb]:false)$Ol[$Gl]=($Gl=="default"?"Neo":ucwords(str_replace("-"," ",$Gl)));}return$Ol;}function
page_headers(){header("Content-Type: text/html; charset=utf-8");header("Cache-Control: no-cache");header("X-XSS-Protection: 0");header("X-Content-Type-Options: nosniff");header("Referrer-Policy: origin-when-cross-origin");header("X-Frame-Options: DENY");$ic=["script-src"=>"'self' 'unsafe-inline' 'nonce-".get_nonce()."' 'strict-dynamic'","connect-src"=>"'self' https://api.github.com/repos/adminneo-org/adminneo/releases/latest","frame-src"=>"'self'","object-src"=>"'none'","base-uri"=>"'none'","form-action"=>"'self'",];Admin::get()->updateCspHeader($ic);$Hc=[];foreach($ic
as$Gc=>$Ik)$Hc[]="$Gc $Ik";header("Content-Security-Policy: ".implode("; ",$Hc));Admin::get()->sendHeaders();}function
get_nonce(){static$xh;if(!$xh)$xh=Random::strongKey();return$xh;}function
page_messages(){$qm=preg_replace('~^[^?]*~','',$_SERVER["REQUEST_URI"]);$Tg=isset($_SESSION["messages"][$qm])?$_SESSION["messages"][$qm]:null;if($Tg){foreach($Tg
as$_)echo"<div class='message'>$_</div>\n",script("initToggles(qsl('.message'));");unset($_SESSION["messages"][$qm]);}foreach(Admin::get()->getErrors()as$i)echo"<div class='error'>$i</div>\n";}function
page_footer($Zg=null){echo"</div>\n","</div>\n","<div id='navigation-panel' class='navigation-panel'>\n","<div class='focus-trap-begin'></div>\n";$dg=isset($_COOKIE["neo_version"])?$_COOKIE["neo_version"]:null;echo"<div class='header'>\n","<button id='close-navigation-button' type='button' class='button light navigation-button' title='",lang(138),"' aria-controls='navigation-panel'>",icon_solo("close"),"</button>",Admin::get()->getServiceTitle()."\n";if($Zg!="auth"){echo"<span class='version'>",h(preg_replace('~\\.0(-|$)~','$1',VERSION));if(Admin::get()->getConfig()->isVersionVerificationEnabled()&&$dg&&version_compare(VERSION,$dg)<0)echo"<a id='version' class='version-badge' href='https://www.adminneo.org/download' ".target_blank()." title='".h($dg)."'>",icon_solo("asterisk"),"</a>";echo"</span>\n";if(Admin::get()->getConfig()->isVersionVerificationEnabled()&&!$dg)echo
script("verifyVersion();");}echo"</div>\n";Admin::get()->printNavigation($Zg);echo"<div class='footer'>\n","<div class='toolbox'>";if($Zg=="auth")language_select();else{$w=h(preg_replace('~\b(db|ns)=[^&]*&~',"",ME)."settings=");echo"<a class='button light' title='",lang(139),"' href='$w'>",icon_solo("settings"),"</a>";}echo"</div>";if($Zg!="auth")Admin::get()->printLogout();echo"</div>\n","<div id='navigation-resizer' class='navigation-resizer'></div>\n","<div class='focus-trap-end'></div>\n","</div>\n",script("initNavigation(); initNavigationResizer('".js_escape(ME)."set=navigation-width', '".get_token()."', ".Settings::$NavigationWidthMin.", ".Settings::$NavigationWidthMax.");");}function
int32($jh){while($jh>=2147483648)$jh-=4294967296;while($jh<=-2147483649)$jh+=4294967296;return(int)$jh;}function
long2str(array$W,$Om){$Qj='';foreach($W
as$X)$Qj
.=pack('V',$X);return$Om?substr($Qj,0,end($W)):$Qj;}function
str2long($Qj,$Om){$W=array_values(unpack('V*',str_pad($Qj,4*ceil(strlen($Qj)/4),"\0")));if($Om)$W[]=strlen($Qj);return$W;}function
xxtea_mx($an,$Zm,$al,$Lf){return
int32((($an>>5&0x7FFFFFF)^$Zm<<2)+(($Zm>>3&0x1FFFFFFF)^$an<<4))^int32(($al^$Zm)+($Lf^$an));}function
xxtea_encrypt_string($Mi,$t){$t=array_values(unpack("V*",pack("H*",md5($t))));$W=str2long($Mi,true);$jh=count($W)-1;$an=$W[$jh];$Zm=$W[0];$jj=floor(6+52/($jh+1));$al=0;while($jj-->0){$al=int32($al+0x9E3779B9);$cd=$al>>2&3;for($qi=0;$qi<$jh;$qi++){$Zm=$W[$qi+1];$hh=xxtea_mx($an,$Zm,$al,$t[$qi&3^$cd]);$an=int32($W[$qi]+$hh);$W[$qi]=$an;}$Zm=$W[0];$hh=xxtea_mx($an,$Zm,$al,$t[$qi&3^$cd]);$an=int32($W[$jh]+$hh);$W[$jh]=$an;}return
long2str($W,false);}function
xxtea_decrypt_string($e,$t){$t=array_values(unpack("V*",pack("H*",md5($t))));$W=str2long($e,false);$jh=count($W)-1;$an=$W[$jh];$Zm=$W[0];$jj=floor(6+52/($jh+1));$al=int32($jj*0x9E3779B9);while($al){$cd=$al>>2&3;for($qi=$jh;$qi>0;$qi--){$an=$W[$qi-1];$hh=xxtea_mx($an,$Zm,$al,$t[$qi&3^$cd]);$Zm=int32($W[$qi]-$hh);$W[$qi]=$Zm;}$an=$W[$jh];$hh=xxtea_mx($an,$Zm,$al,$t[$qi&3^$cd]);$Zm=int32($W[0]-$hh);$W[0]=$Zm;$al=int32($al-0x9E3779B9);}return
long2str($W,true);}const
ENCRYPTION_GCM='aes-256-gcm';const
ENCRYPTION_CBC='aes-256-cbc';const
ENCRYPTION_TAG_LENGTH=16;const
ENCRYPTION_HMAC_LENGTH=64;function
generate_iv($u){if(function_exists('random_bytes')){try{return
random_bytes($u);}catch(Exception$cd){}}return
openssl_random_pseudo_bytes($u);}function
hash_key($t){return
substr(hash('sha512',$t,true),0,32);}function
aes_encrypt_string($Mi,$t){$Xg=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$t=hash_key($t);$Gf=generate_iv(openssl_cipher_iv_length($Xg)?:16);if($Xg==ENCRYPTION_GCM)$xb=openssl_encrypt($Mi,$Xg,$t,OPENSSL_RAW_DATA,$Gf,$yl,"",ENCRYPTION_TAG_LENGTH);else{$xb=openssl_encrypt($Mi,$Xg,$t,OPENSSL_RAW_DATA,$Gf);$yl=hash_hmac("sha512",$Gf.$xb,$t,true);}if($xb===false)return
false;return$Gf.$yl.$xb;}function
aes_decrypt_string($e,$t){$Xg=PHP_VERSION_ID>=70100&&in_array(ENCRYPTION_GCM,openssl_get_cipher_methods())?ENCRYPTION_GCM:ENCRYPTION_CBC;$Hf=openssl_cipher_iv_length($Xg)?:16;$zl=$Xg==ENCRYPTION_GCM?ENCRYPTION_TAG_LENGTH:ENCRYPTION_HMAC_LENGTH;if(strlen($e)<$Hf+$zl)return
false;$t=hash_key($t);$Gf=substr($e,0,$Hf);$yl=substr($e,$Hf,$zl);$xb=substr($e,$Hf+$zl);if($Gf===false||$yl===false||$xb===false)return
false;if($Xg==ENCRYPTION_GCM)return
openssl_decrypt($xb,$Xg,$t,OPENSSL_RAW_DATA,$Gf,$yl);else{$Se=hash_hmac('sha512',$Gf.$xb,$t,true);if(!hash_equals($yl,$Se))return
false;return
openssl_decrypt($xb,$Xg,$t,OPENSSL_RAW_DATA,$Gf);}}function
encrypt_string($Mi,$t){if($Mi=="")return"";if(extension_loaded('openssl'))return
aes_encrypt_string($Mi,$t);else
return
xxtea_encrypt_string($Mi,$t);}function
decrypt_string($e,$t){if($e=="")return"";if(extension_loaded('openssl'))return
aes_decrypt_string($e,$t);else
return
xxtea_decrypt_string($e,$t);}$Ji=[];if($_COOKIE["neo_permanent"]){foreach(explode(" ",$_COOKIE["neo_permanent"])as$X){list($t)=explode(":",$X);$Ji[$t]=$X;}}function
validate_server_input(array&$Ji){$N=preg_replace('~:/[-\w.][-\w.:/]*$~D',"",SERVER);if($N=="")return;if(!preg_match('~^[^:]+://~',$N))$N="https://$N";$Di=parse_url($N);if(!$Di)auth_error($Ji);if(isset($Di['user'])||isset($Di['pass'])||isset($Di['query'])||isset($Di['fragment']))auth_error($Ji);if(isset($Di['scheme'])&&!preg_match('~^(https?)$~i',$Di['scheme']))auth_error($Ji);$Ve=$Di['host'].(isset($Di['path'])?$Di['path']:'');if(!is_server_host_valid($Ve))auth_error($Ji);if(isset($Di['port'])&&($Di['port']<1024||$Di['port']>65535))auth_error($Ji,lang(140));}if(!function_exists('AdminNeo\is_server_host_valid')){function
is_server_host_valid($Ve){return
strpos($Ve,'/')===false;}}function
build_http_url($N,$V,$F,$xc,$wc=null){if(!preg_match('~^(https?://)?([^:]*)(:\d+)?$~',rtrim($N,'/'),$z))return
null;return($z[1]?:"http://").($V!==""||$F!==""?urlencode($V).":".urlencode($F)."@":"").($z[2]!==""?$z[2]:$xc).(isset($z[3])?$z[3]:($wc?":$wc":""));}function
add_invalid_login(){$Xa=get_temp_dir()."/adminneo-invalid";$l=null;foreach(glob("$Xa*")?:[$Xa]as$m){$l=open_file_with_lock($m);if($l)break;}if(!$l){$l=open_file_with_lock("$Xa-".Random::strongKey());if(!$l)return;}$xf=json_decode(stream_get_contents($l),true);$Jl=time();if($xf){foreach($xf
as$yf=>$X){if($X[0]<$Jl)unset($xf[$yf]);}}$wf=&$xf[Admin::get()->getBruteForceKey()];if(!$wf)$wf=[$Jl+30*60,0];$wf[1]++;write_and_unlock_file($l,json_encode($xf));}function
check_invalid_login(array&$Ji){$Xa=get_temp_dir()."/adminneo-invalid";$xf=[];foreach(glob("$Xa*")as$m){$l=open_file_with_lock($m);if($l){$xf=json_decode(stream_get_contents($l),true);unlock_file($l);break;}}$wf=($xf?$xf[Admin::get()->getBruteForceKey()]:[]);$vh=($wf&&$wf[1]>29?$wf[0]-time():0);if($vh>0)auth_error($Ji,lang(141,ceil($vh/60)));}function
connect_to_db(array&$Ji){if(Admin::get()->getConfig()->hasServers()&&!Admin::get()->getConfig()->getServer(SERVER))auth_error($Ji);$d=connect(true,$i);if(!$d)connection_error(nl2br(h($i)),$Ji);return$d;}function
authenticate(array&$Ji){$I=Admin::get()->authenticate($_GET["username"],get_password());if($I!==true)connection_error($I,$Ji);}function
connection_error($i,array&$Ji){$i=$i?:lang(3);if(preg_match('~^ +| +$~',get_password()))$i
.="<br>".lang(142);auth_error($Ji,$i);}Admin::get()->init();$Na=isset($_POST["auth"])?$_POST["auth"]:null;if($Na){session_regenerate_id();$N=isset($Na["server"])?$Na["server"]:"";$rk=Admin::get()->getConfig()->getServer($N);$Tc=$rk?$rk->getDriver():(isset($Na["driver"])?$Na["driver"]:"");$N=$rk?$N:trim($N);$V=isset($Na["username"])?$Na["username"]:"";$F=isset($Na["password"])?$Na["password"]:"";if($rk&&$rk->hasCredentials()&&$V==""&&$F==""){$V=$rk->getUsername();$F=$rk->getPassword();}$g=$rk?$rk->getDatabase():(isset($Na["db"])?$Na["db"]:"");save_login($Tc,$N,$V,$F,$g);if($Na["permanent"]){$t=implode("-",array_map("base64_encode",[$Tc,$N,$V,$g]));$cj=Admin::get()->getPrivateKey(true);$nd=$cj?encrypt_string($F,$cj):false;$Ji[$t]="$t:".base64_encode($nd?:"");cookie("neo_permanent",implode(" ",$Ji));}if(count($_POST)==1||DRIVER!=$Tc||SERVER!=$N||$_GET["username"]!==$V||DB!=$g)redirect(auth_url($Tc,$N,$V,$g));}elseif($_POST["logout"]&&(!$_SESSION["token"]||verify_token())){foreach(["pwds","db","dbs","queries"]as$t)set_session($t,null);unset_permanent($Ji);redirect(SERVER_HOME_URL,lang(143));}elseif($Ji&&!$_SESSION["pwds"]){session_regenerate_id();$cj=Admin::get()->getPrivateKey();foreach($Ji
as$t=>$X){list(,$wb)=explode(":",$X);list($Tc,$N,$V,$g)=array_map("base64_decode",explode("-",$t));$F=$cj?decrypt_string(base64_decode($wb),$cj):false;save_login($Tc,$N,$V,$F,$g);}}function
unset_permanent(array&$Ji){foreach($Ji
as$t=>$X){list($Tc,$N,$V,$g)=array_map("base64_decode",explode("-",$t));if($Tc==DRIVER&&$N==SERVER&&$V==$_GET["username"]&&$g==DB)unset($Ji[$t]);}cookie("neo_permanent",implode(" ",$Ji));}function
auth_error(array&$Ji,$i=null){$vk=session_name();if(isset($_GET["username"])){header("HTTP/1.1 403 Forbidden");if(($_COOKIE[$vk]||$_GET[$vk])&&!$_SESSION["token"])$i=lang(144);else{restart_session();add_invalid_login();$F=get_password();if($F!==null){if($F===false)$i=lang(145);delete_login(DRIVER,SERVER,$_GET["username"]);}unset_permanent($Ji);}}if(!$_COOKIE[$vk]&&$_GET[$vk]&&ini_bool("session.use_only_cookies"))$i=lang(146);if(!$i)$i=lang(3);Admin::get()->addError($i);print_login_page();}function
print_login_page(){$ti=session_get_cookie_params();cookie("neo_key",($_COOKIE["neo_key"]?:Random::strongKey()),$ti["lifetime"]);if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);page_header(lang(32),null);echo"<form action='' method='post'>\n","<div>";if(print_hidden_fields($_POST,["auth"]))echo"<p class='message'>".lang(147)."\n";echo"</div>\n";Admin::get()->printLoginForm();echo"</form>\n";page_footer("auth");exit;}if(isset($_GET["username"])&&!DRIVER)print_login_page();if(isset($_GET["username"])&&!defined('AdminNeo\DRIVER_EXTENSION')){Admin::get()->addError(lang(148,implode(", ",Drivers::getExtensions(DRIVER))));unset($_SESSION["pwds"][DRIVER]);unset_permanent($Ji);page_header(lang(149),false);page_footer("auth");exit;}if(!isset($_GET["username"])||get_password()===null)print_login_page();validate_server_input($Ji);check_invalid_login($Ji);Admin::get()->getConfig()->applyServer(SERVER);$d=connect_to_db($Ji);authenticate($Ji);create_driver($d);if($_POST["logout"]&&$_SESSION["token"]&&!verify_token()){Admin::get()->addError(lang(150));page_header(lang(7));page_footer("db");exit;}if(!$_SESSION["token"])$_SESSION["token"]=rand(1,1e6);stop_session(true);if($Na&&$_POST["token"])$_POST["token"]=get_token();if($_POST){if(!verify_token()){Admin::get()->addError(lang(150).' '.lang(151));$_POST=[];}}elseif($_SERVER["REQUEST_METHOD"]=="POST"){$i=lang(152,"'post_max_size'");if(isset($_GET["sql"]))$i
.=' '.lang(153);Admin::get()->addError($i);}if(isset($_GET["settings"])){$O=Admin::get()->getSettings();$yk=array_merge(Admin::get()->getSettingsRows(1),Admin::get()->getSettingsRows(2),Admin::get()->getSettingsRows(3));if($_POST){$ti=[];foreach($yk
as$t=>$K){if(isset($_POST[$t])){$tm=$_POST[$t]===""||(is_array($_POST[$t])&&in_array("",$_POST[$t]));$ti[$t]=(!$tm?$_POST[$t]:null);}}$O->updateParameters($ti);redirect(remove_from_uri());}$T=lang(139);page_header($T,[$T]);echo"<form id='settings' action='' method='post'>\n","<table class='box'>\n";foreach($yk
as$K)echo$K;echo"</table>\n","<p>","<input type='submit' value='".lang(115),"' class='button default hidden'>",input_token(),"</p>\n","</form>\n",script("initSettingsForm();");page_footer();exit;}if(isset($_GET["status"]))$_GET["variables"]=$_GET["status"];if(isset($_GET["import"]))$_GET["sql"]=$_GET["import"];if(DB==""&&isset($_GET["ns"]))redirect(remove_from_uri('ns'));if(!(DB!=""?Connection::get()->selectDatabase(DB):(isset($_GET["sql"])||isset($_GET["dump"])||isset($_GET["database"])||isset($_GET["processlist"])||isset($_GET["privileges"])||isset($_GET["user"])||isset($_GET["variables"])||$_GET["script"]=="connect"||$_GET["script"]=="kill"))){if(DB!=""||$_GET["refresh"]){restart_session();set_session("dbs",null);}if(DB!=""){Admin::get()->addError(lang(154));header("HTTP/1.1 404 Not Found");page_header(lang(31).": ".h(DB),true);}else{if($_POST["db"])queries_redirect(substr(ME,0,-1),lang(155),drop_databases($_POST["db"]));$T=h(Drivers::get(DRIVER).": ".Admin::get()->getServerName(SERVER));page_header($T,false);$rg=['privileges'=>[lang(73),"users"],'processlist'=>[lang(156),"list"],'variables'=>[lang(157),"variable"],'status'=>[lang(158),"status"],];$sg="";foreach($rg
as$t=>$X){if(support($t))$sg
.="<a href='".h(ME)."$t='>".icon($X[1])."$X[0]</a>";}if($sg)echo"<p class='links top-links'>$sg</p>\n";echo"<p>".lang(159,Drivers::get(DRIVER),"<b>".h(Connection::get()->getVersion())."</b>","<b>".DRIVER_EXTENSION."</b>")."\n","<p>".lang(160,"<b>".h(logged_user())."</b>")."\n";$f=Admin::get()->getDatabases();if($f){$ak=support("scheme");$Da=collations();echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr>".(support("database")?"<th>":"")."<th aria-sort='ascending'>".lang(31).(get_session("dbs")!==null?" - <a href='".h(ME)."refresh=1'>".lang(161)."</a>":"")."<td>".lang(46)."<td>".lang(162)."<td>".lang(163)." - <a href='".h(ME)."dbsize=1'>".lang(164)."</a>".script("qsl('a').onclick = partial(ajaxSetHtml, '".js_escape(ME)."script=connect');","")."</thead>\n","<tbody>\n";$f=($_GET["dbsize"]?count_tables($f):array_flip($f));foreach($f
as$g=>$S){$Lj=h(ME)."db=".urlencode($g);$q=h("Db-".$g);echo"<tr>".(support("database")?"<th class='actions'>".checkbox("db[]",$g,in_array($g,(array)$_POST["db"]),"","","",$q):""),"<th><a href='$Lj' id='$q'>".h($g)."</a>";$Bb=h(db_collation($g,$Da));echo"<td>".(support("database")?"<a href='$Lj".($ak?"&amp;ns=":"")."&amp;database=' title='".lang(70)."'>$Bb</a>":$Bb),"<td align='right'><a href='$Lj&amp;schema=' id='tables-".h($g)."' title='".lang(72)."'>".($_GET["dbsize"]?$S:"?")."</a>","<td align='right' id='size-".h($g)."'>".($_GET["dbsize"]?db_size($g):"?"),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});"),"</table>\n","</div>\n";if(support("database"))echo"<div class='table-footer'><div class='field-sets'>\n","<fieldset><legend>",lang(165)," <span id='selected'></span></legend><div class='fieldset-content'>\n",input_hidden("all"),script("qsl('input').onclick = countDbs;"),"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(),"\n","</div></fieldset>\n","</div></div>\n",script("initTableFooter()");echo"</div>\n",input_token(),"</form>\n",script("tableCheck();");}}echo'<p class="links"><a href="'.h(ME).'database=">'.icon("database-add").lang(76)."</a>\n";page_footer("db");exit;}if(isset($_GET["select"])&&($_POST["edit"]||$_POST["clone"])&&!$_POST["save"])$_GET["edit"]=$_GET["select"];if(isset($_GET["callf"]))$_GET["call"]=$_GET["callf"];if(isset($_GET["function"]))$_GET["procedure"]=$_GET["function"];if(isset($_GET["download"])){$a=$_GET["download"];$k=fields($a);header("Content-Type: application/octet-stream");header("Content-Disposition: attachment; filename=".friendly_url("$a-".implode("_",$_GET["where"])).".".friendly_url($_GET["field"]));$M=[idf_escape($_GET["field"])];$I=Driver::get()->select($a,$M,[where($_GET,$k)],$M);$K=($I?$I->fetchRow():[]);echo
Connection::get()->formatValue($K[0],$k[$_GET["field"]]);exit;}elseif(isset($_GET["table"])){$a=$_GET["table"];$k=fields($a);if(!$k)Admin::get()->addError(error()?:lang(79));$R=table_status1($a,true);$A=Admin::get()->getTableName($R);$Kj=[];foreach($k
as$t=>$j)$Kj+=$j["privileges"];$T=$k&&is_view($R)?$R['Engine']=='materialized view'?lang(167):lang(168):lang(9);$ol=$A!=""?$A:h($a);page_header("$T: $ol",[$ol]);$sf=null;if(isset($Kj["insert"])||!support("table"))$sf=[];Admin::get()->printTableMenu($R,$sf);$kf=[];if(!preg_match("~sqlite|mssql|pgsql~",DIALECT)&&isset($R["Engine"]))$kf[]=lang(169).": ".h($R["Engine"]);if(isset($R["Collation"]))$kf[]=lang(46).": ".h($R["Collation"]);if($kf)echo"<p>",implode(", ",$kf),"</p>";if($k)Admin::get()->printTableStructure($k);$Lb=$R["Comment"];if($Lb!="")echo"<p class='keep-lines'>",lang(47),": ",Admin::get()->formatComment($Lb),"</p>\n";if(!is_view($R))$ed='<p class="links"><a href="'.h(ME).'create='.urlencode($a).'">'.icon("edit").lang(36)."</a>\n";elseif(support("view"))$ed='<p class="links"><a href="'.h(ME).'view='.urlencode($a).'">'.icon("edit").lang(37)."</a>\n";else$ed="";if($kf||$k||$Lb!="")echo$ed;$ui=Driver::get()->getParentTables($a);if($ui){echo"<h2>".lang(170)."</h2>\n";Admin::get()->printRelatedTables($ui);}if(Driver::get()->getPartitionBy()&&str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned")){$Ci=Driver::get()->getPartitionsInfo($a);if($Ci){echo"<h2 id='partitions'>".lang(50)."</h2>\n";Admin::get()->printTablePartitions($Ci);if(DIALECT!="pgsql")echo$ed;}}$lf=Driver::get()->getInheritedTables($a);if($lf){echo"<h2 id='inherited-by'>".lang(171)."</h2>\n";Admin::get()->printRelatedTables($lf);}if(support("indexes")&&Driver::get()->supportsIndex($R)){echo"<h2 id='indexes'>".lang(172)."</h2>\n";$s=indexes($a);if($s)Admin::get()->printTableIndexes($s,$R);echo'<p class="links"><a href="'.h(ME).'indexes='.urlencode($a).'">'.icon("edit").lang(173)."</a>\n";}if(!is_view($R)){if(fk_support($R)){echo"<h2 id='foreign-keys'>".lang(91)."</h2>\n";$he=foreign_keys($a);if($he){echo"<table>\n","<thead><tr><th>".lang(174)."<td>".lang(175)."<td>".lang(94)."<td>".lang(93)."<td></thead>\n";foreach($he
as$A=>$n)echo"<tr title='".h($A)."'>","<th><i>".implode("</i>, <i>",array_map('AdminNeo\h',$n["source"]))."</i>","<td><a href='".h($n["db"]!=""?preg_replace('~db=[^&]*~',"db=".urlencode($n["db"]),ME):($n["ns"]!=""?preg_replace('~ns=[^&]*~',"ns=".urlencode($n["ns"]),ME):ME))."table=".urlencode($n["table"])."'>".($n["db"]!=""&&$n["db"]!=DB?"<b>".h($n["db"])."</b>.":"").($n["ns"]!=""&&$n["ns"]!=$_GET["ns"]?"<b>".h($n["ns"])."</b>.":"").h($n["table"])."</a>","(<i>".implode("</i>, <i>",array_map('AdminNeo\h',$n["target"]))."</i>)","<td>".h($n["on_delete"]),"<td>".h($n["on_update"]),'<td><a href="'.h(ME.'foreign='.urlencode($a).'&name='.urlencode($A)).'">'.lang(176).'</a>',"\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'foreign='.urlencode($a).'">'.icon("add").lang(177)."</a>\n";}if(support("check")){echo"<h2 id='checks'>".lang(178)."</h2>\n";$rb=Driver::get()->checkConstraints($a);if($rb){echo"<table cellspacing='0'>\n";foreach($rb
as$t=>$X)echo"<tr title='".h($t)."'>","<td><code class='jush-".DIALECT."'>".truncate_utf8(preg_replace('~\s+~',' ',ltrim($X))),"<td><a href='".h(ME.'check='.urlencode($a).'&name='.urlencode($t))."'>".lang(176)."</a>","\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'check='.urlencode($a).'">'.icon("add").lang(179)."</a>\n";}}if(support(is_view($R)?"view_trigger":"trigger")){echo"<h2 id='triggers'>".lang(180)."</h2>\n";$am=triggers($a);if($am){echo"<table>\n";foreach($am
as$t=>$X)echo"<tr><td>".h($X[0])."<td>".h($X[1])."<th>".h($t)."<td><a href='".h(ME.'trigger='.urlencode($a).'&name='.urlencode($t))."'>".lang(176)."</a>\n";echo"</table>\n";}echo'<p class="links"><a href="'.h(ME).'trigger='.urlencode($a).'">'.icon("add").lang(181)."</a>\n";}}elseif(isset($_GET["schema"])){$Ml=h(": ".DB.($_GET["ns"]?".$_GET[ns]":""));page_header(lang(72).$Ml,[lang(72)]);$ql=[];$rl=[];$Qd=[];$pa=($_GET["schema"]?:$_COOKIE["neo_schema-".str_replace(".","_",DB)]);preg_match_all('~([^:]+):([-0-9.]+)x([-0-9.]+)(_|$)~',$pa,$z,PREG_SET_ORDER);foreach($z
as$y){$ql[$y[1]]=[(float)$y[2],(float)$y[3]];$rl[]="\n'".js_escape($y[1])."': [ $y[2], $y[3] ]";}$og=1.4;$Sl=0;$Wa=-1;$Yj=[];$yj=[];$gg=[];$Ea=Driver::get()->getAllFields();foreach(table_status('',true)as$Q=>$R){if(is_view($R))continue;$G=0;$Yj[$Q]["fields"]=[];foreach(isset($Ea[$Q])?$Ea[$Q]:[]as$j){$G=round($G+$og,2);$Qd[$Q][$j["field"]]=$G;$Yj[$Q]["fields"][$j["field"]]=$j;}$Yj[$Q]["pos"]=(isset($ql[$Q])?$ql[$Q]:[$Sl,0]);foreach(Admin::get()->getForeignKeys($Q)as$X){if(!$X["db"]){$eg=$Wa;if((isset($ql[$Q][1])?$ql[$Q][1]:0)||(isset($ql[$X["table"]][1])?$ql[$X["table"]][1]:0))$eg=min(floatval(isset($ql[$Q][1])?$ql[$Q][1]:0),floatval(isset($ql[$X["table"]][1])?$ql[$X["table"]][1]:0))-1;else$Wa-=.1;while($gg[(string)$eg])$eg-=.0001;$Yj[$Q]["references"][$X["table"]][(string)$eg]=[$X["source"],$X["target"]];$yj[$X["table"]][$Q][(string)$eg]=$X["target"];$gg[(string)$eg]=true;}}$Sl=max($Sl,round($Yj[$Q]["pos"][0]+$og+$G+1,2));}echo"<div id='schema' style='height: {$Sl}em;'>\n";foreach($Yj
as$A=>$Q){$Me=round($og+count($Q["fields"])*$og+0.4,2);echo"<div class='table' style='top: ".$Q["pos"][0]."em; left: ".$Q["pos"][1]."em; height: {$Me}em;'>","<div class='content'>",'<h4><a href="'.h(ME).'table='.urlencode($A).'">'.h($A)."</a></h4>","<ul>";foreach($Q["fields"]as$j){$X='<span '.type_class($j["type"]).' title="'.h($j["type"].($j["length"]?"($j[length])":"").($j["null"]?" NULL":'')).'">'.h($j["field"]).'</span>';echo"<li>".($j["primary"]?"<i>$X</i>":$X)."</li>";}echo"</ul>","</div>";foreach((array)$Q["references"]as$Al=>$_j){foreach($_j
as$eg=>$uj){$fg=$eg-(isset($ql[$A][1])?$ql[$A][1]:0);$p=0;foreach($uj[0]as$Hk){echo"\n<div class='references outgoing' title='",h($Al),"' id='refs$eg-$p' style='left: {$fg}em; top: ",$Qd[$A][$Hk],"em;'>","<div style='width: ".(-$fg)."em;'></div>","</div>";$p++;}}}foreach((array)$yj[$A]as$Al=>$_j){foreach($_j
as$eg=>$c){$fg=$eg-(isset($ql[$A][1])?$ql[$A][1]:0);$p=0;foreach($c
as$_l){echo"\n<div class='references incoming' title='",h($Al),"' id='refd$eg-$p' style='left: {$fg}em; top: ".$Qd[$A][$_l]."em;'>","<svg viewBox='0 0 22 22' fill='currentColor'><path d='M11,19l10,-8l-10,-8l0,16Z'/></svg>","<div style='width: ".(-$fg)."em;'></div>","</div>";$p++;}}}echo"\n</div>\n";}foreach($Yj
as$A=>$Q){foreach((array)$Q["references"]as$Al=>$_j){if($Yj[$Al]){foreach($_j
as$eg=>$uj){$Yg=$Sl;$Ig=-10;foreach($uj[0]as$t=>$Hk){$Si=$Q["pos"][0]+$Qd[$A][$Hk];$Ti=$Yj[$Al]["pos"][0]+$Qd[$Al][$uj[1][$t]];$Yg=round(min($Yg,$Si,$Ti),2);$Ig=round(max($Ig,$Si,$Ti),2);}echo"<div class='references vertical' id='refl$eg' style='left: $eg"."em; top: $Yg"."em;'>"."<div style='height: ".round($Ig-$Yg,2)."em;'></div></div>\n";}}}}echo"</div>\n",script("initSchema('".js_escape(DB)."', $Sl, {".implode(",",$rl)."})"),"<p class='links'>","<a href='",(ME."schema=".urlencode($pa)),"' id='schema-link'>",lang(182),"</a>","</p>\n";}elseif(isset($_GET["dump"])){$a=$_GET["dump"];$O=Admin::get()->getSettings();if($_POST){$O->updateParameters(["dumpFormat"=>$_POST["format"],"dumpDbStyle"=>$_POST["db_style"],"dumpTypes"=>isset($_POST["types"])?$_POST["types"]:(support("type")?"":null),"dumpRoutines"=>isset($_POST["routines"])?$_POST["routines"]:(support("routine")?"":null),"dumpEvents"=>isset($_POST["events"])?$_POST["events"]:(support("event")?"":null),"dumpTableStyle"=>$_POST["table_style"],"dumpAutoIncrement"=>isset($_POST["auto_increment"])?$_POST["auto_increment"]:"","dumpTriggers"=>isset($_POST["triggers"])?$_POST["triggers"]:(support("trigger")?"":null),"dumpDataStyle"=>$_POST["data_style"],"dumpOutput"=>$_POST["output"],]);if(DB!="")$f=[DB];else{$f=isset($_POST["databases"])?$_POST["databases"]:[];if(is_string($f))$f=explode("\n",rtrim(str_replace("\r","",$f),"\n"));}$Zj=isset($_POST["schemas"])?$_POST["schemas"]:[];$S=array_flip(isset($_POST["tables"])?$_POST["tables"]:[])+array_flip(isset($_POST["data"])?$_POST["data"]:[]);if(count($S)==1)$Ze=key($S);elseif(count($Zj)==1)$Ze=$Zj[0];elseif(count($f)==1)$Ze=$f[0];else$Ze=Admin::get()->getServerName(SERVER,true,"server");$Ed=dump_headers($Ze,DB==""||$_GET["ns"]===""||count($S)>1);$Df=preg_match('~sql~',$_POST["format"]);$oc=$Df&&$_POST["data_style"]&&!$_POST["table_style"]&&DIALECT!="sql";if($Df){echo"-- AdminNeo ".VERSION." ".Drivers::get(DRIVER)." ".Connection::get()->getVersion()." dump\n\n";if(DIALECT=="sql"){echo"SET NAMES utf8;
SET time_zone = '+00:00';
SET foreign_key_checks = 0;
".($_POST["data_style"]?"SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO';
":"")."
";Connection::get()->query("SET time_zone = '+00:00'");Connection::get()->query("SET sql_mode = ''");}}$Wk=$_POST["db_style"];foreach($f
as$g){Admin::get()->dumpDatabase($g);if(Connection::get()->selectDatabase($g)){if($Df){if($Wk)echo
create_database_sql($g,$Wk),use_sql($g,$Wk)."\n";$ni="";if($_POST["types"]){foreach(types()as$q=>$U){$rd=type_values($q);if($rd)$ni
.=($Wk!='DROP+CREATE'?"DROP TYPE IF EXISTS ".idf_escape($U).";;\n":"")."CREATE TYPE ".idf_escape($U)." AS ENUM ($rd);\n\n";else$ni
.="-- Could not export type $U\n\n";}}if($_POST["routines"]){foreach(routines()as$K){$A=$K["ROUTINE_NAME"];$Mj=$K["ROUTINE_TYPE"];$ec=create_routine($Mj,["name"=>$A]+routine($K["SPECIFIC_NAME"],$Mj));set_utf8mb4($ec);$ni
.=($Wk!='DROP+CREATE'?"DROP $Mj IF EXISTS ".idf_escape($A).";;\n":"")."$ec;\n\n";}}if($_POST["events"]){foreach(get_rows("SHOW EVENTS",null,"-- ")as$K){$ec=remove_definer(Connection::get()->getValue("SHOW CREATE EVENT ".idf_escape($K["Name"]),3));set_utf8mb4($ec);$ni
.=($Wk!='DROP+CREATE'?"DROP EVENT IF EXISTS ".idf_escape($K["Name"]).";;\n":"")."$ec;;\n\n";}}echo($ni&&DIALECT=='sql'?"DELIMITER ;;\n\n$ni"."DELIMITER ;\n\n":$ni);}if($_POST["table_style"]||$_POST["data_style"]){foreach(($_GET["ns"]===""?(array)$_POST["schemas"]:(DB!=""||!support("scheme")?[""]:Admin::get()->getSchemas(true)))as$Yj){if($Yj!="")set_schema($Yj);$wl=table_status('',true);$pl=array_keys($wl);$Ic=false;if($oc&&$pl){$zj=[];foreach($pl
as$A){if(!is_view($wl[$A])&&(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]))){foreach(foreign_keys($A)as$n)$zj[$A][]=$n["table"];}}$ci=dump_table_order($pl,$zj);if($ci)$pl=$ci;else$Ic=function_exists('AdminNeo\foreign_key_checks_sql');}if($Ic)echo
foreign_key_checks_sql(false)."\n";$Jm=[];foreach($pl
as$A){$R=$wl[$A];$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));$e=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["data"]));if($Q||$e){$Pl=null;if($Ed=="tar"){$Pl=new
TmpFile();ob_start([$Pl,'write'],1e5);}$fc=($Q?$_POST["table_style"]:"");Admin::get()->dumpTable($A,$fc,(is_view($R)?2:0));if(is_view($R)&&$Ed!="tar")$Jm[]=$A;elseif($e){$k=fields($A);Admin::get()->dumpData($A,$_POST["data_style"],"SELECT *".convert_fields($k,$k)." FROM ".table($A));if($Df&&!$fc&&$_POST["auto_increment"]&&function_exists('AdminNeo\restart_sequences_sql'))echo"\n".restart_sequences_sql($A);}if($Df&&$_POST["triggers"]&&$Q&&($am=trigger_sql($A)))echo"\nDELIMITER ;;\n$am\nDELIMITER ;\n";if($Ed=="tar"){ob_end_flush();tar_file((DB!=""?"":"$g/")."$A.csv",$Pl);}elseif($Df)echo"\n";}}if($Ic)echo
foreign_key_checks_sql(true)."\n";if($_POST["table_style"]&&function_exists('AdminNeo\foreign_keys_sql')){foreach($wl
as$A=>$R){$Q=(DB==""||$_GET["ns"]===""||in_array($A,(array)$_POST["tables"]));if($Q&&!is_view($R))echo
foreign_keys_sql($A);}}foreach($Jm
as$Hm)Admin::get()->dumpTable($Hm,$_POST["table_style"],1);if($Ed=="tar")echo
pack("x512");}}}}if($Df)echo"-- ".gmdate("Y-m-d H:i:s e")."\n";exit;}$A=DB!=""?h(DB):h(Admin::get()->getServerName(SERVER));page_header(lang(75).": $A",($_GET["export"]!=""?["table"=>$_GET["export"]]:[lang(75)]));echo"<form action='' method='post'>\n","<table class='box'>\n";$sc=['','USE','DROP+CREATE','CREATE'];$tl=['','DROP+CREATE','CREATE'];$pc=['','TRUNCATE+INSERT','INSERT'];if(DIALECT=="sql")$pc[]='INSERT+UPDATE';echo"<tr><th>",lang(183),"</th><td>",html_radios("format",Admin::get()->getDumpFormats(),$O->getParameter("dumpFormat","sql")),"</td></tr>\n";if(DIALECT!="sqlite"){echo"<tr><th id='label-db'>",lang(31),"</th>","<td>",html_select('db_style',$sc,$O->getParameter("dumpDbStyle",DB==""?"CREATE":""),"","label-db"),"<span class='labels'>";if(support("routine"))echo
checkbox("routines",1,$O->getParameter("dumpRoutines",$_GET["dump"]==""?"1":""),lang(184));if(support("event"))echo
checkbox("events",1,$O->getParameter("dumpEvents",$_GET["dump"]==""?"1":""),lang(185));echo"</span></td></tr>";}echo"<tr><th id='label-tables'>",lang(162),"</th><td>",html_select('table_style',$tl,$O->getParameter("dumpTableStyle","DROP+CREATE"),"","label-tables")," <span class='labels'>",checkbox("auto_increment",1,$O->getParameter("dumpAutoIncrement"),lang(48));if(support("trigger"))echo
checkbox("triggers",1,$O->getParameter("dumpTriggers","1"),lang(180));echo"</span></td></tr>","<tr><th id='label-data'>",lang(186),"</th><td>",html_select("data_style",$pc,$O->getParameter("dumpDataStyle","INSERT"),"","label-data"),"</td></tr>","<tr><th>",lang(187),"</th><td>",html_radios("output",Admin::get()->getDumpOutputs(),$O->getParameter("dumpOutput","file")),"</td></tr>\n","</table>\n","<p>","<input type='submit' class='button default' value='",lang(75),"'>",input_token(),"</p>\n","<table>\n",script("qsl('table').onclick = dumpClick;");$Yi=[];if(DB!=""&&$_GET["ns"]===""){echo"<thead><tr><th>","<label class='block'><input type='checkbox' id='check-schemas' checked class='jsonly' title='".lang(188)."'>".lang(189)."</label>".script("gid('check-schemas').onclick = partial(formCheck, /^schemas\\[/);",""),"</thead>\n";foreach(Admin::get()->getSchemas()as$Yj){if(!information_schema(DB,$Yj))echo"<tr><td>".checkbox("schemas[]",$Yj,true,$Yj,"","block")."\n";}}elseif(DB!=""){$tb=($a!=""?"":" checked");echo"<thead><tr>","<th><label class='block'><input type='checkbox' id='check-tables'$tb class='jsonly' title='".lang(188)."'>".lang(9)."</label>".script("gid('check-tables').onclick = partial(formCheck, /^tables\\[/);",""),"<th class='right'><label class='block'>".lang(186)."<input type='checkbox' id='check-data'$tb class='jsonly' title='".lang(188)."'></label>".script("gid('check-data').onclick = partial(formCheck, /^data\\[/);",""),"</thead>\n";$Jm="";$vl=tables_list();foreach($vl
as$A=>$U){$Xi=preg_replace('~_.*~','',$A);$tb=($a==""||$a==(substr($a,-1)=="%"?"$Xi%":$A));$bj="<tr><td>".checkbox("tables[]",$A,$tb,$A,"","block");if($U!==null&&!preg_match('~table~i',$U))$Jm
.="$bj\n";else
echo"$bj<td class='right'><label class='block'><span id='Rows-".h($A)."'></span>".checkbox("data[]",$A,$tb)."</label>\n";$Yi[$Xi]++;}echo$Jm;if($vl)echo
script("ajaxSetHtml('".js_escape(ME)."script=db');");}else{$f=Admin::get()->getDatabases();echo"<thead><tr><th>","<label class='block'>".($f?"<input type='checkbox' id='check-databases'".($a==""?" checked":"")." class='jsonly' title='".lang(188)."'>".script("gid('check-databases').onclick = partial(formCheck, /^databases\\[/);",""):"").lang(31)."</label>","</thead>\n";if($f){foreach($f
as$g){if(!information_schema($g)){$Xi=preg_replace('~_.*~','',$g);echo"<tr><td>".checkbox("databases[]",$g,$a==""||$a=="$Xi%",$g,"","block")."\n";$Yi[$Xi]++;}}}else
echo"<tr><td><textarea name='databases' rows='10' cols='20'></textarea>";}echo"</table>\n","</form>\n";$rg=[];foreach($Yi
as$t=>$X){if($t!=""&&$X>1)$rg[]="<a href='".h(ME)."dump=".urlencode("$t%")."'>".icon("check").h($t)."*</a>";}if($rg)echo"<p class='links'>",implode("",$rg),"</p>\n";}elseif(isset($_GET["privileges"])){$Ml=DB!=""?h(": ".DB):"";page_header(lang(73).$Ml,[lang(73)]);echo'<p class="links top-links"><a href="',h(ME),'user=">',icon("user-add"),lang(190),"</a></p>\n";$I=Connection::get()->query("SELECT User, Host FROM mysql.".(DB==""?"user":"db WHERE ".q(DB)." LIKE Db")." ORDER BY Host, User");$xe=$I;if(!$I)$I=Connection::get()->query("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', 1) AS User, SUBSTRING_INDEX(CURRENT_USER, '@', -1) AS Host");echo"<form action=''>\n";hidden_fields_get();echo
input_hidden("db",DB);if(!$xe)echo
input_hidden("grant");echo"\n","<div class='scrollable'>\n","<table class='checkable'>\n","<thead><tr><th>".lang(6)."<th>".lang(5)."<th></thead>\n";while($K=$I->fetchAssoc())echo'<tr><td>'.h($K["User"])."<td>".h($K["Host"]).'<td><a href="'.h(ME.'user='.urlencode($K["User"]).'&host='.urlencode($K["Host"])).'">'.lang(39)."</a>\n";if(!$xe||DB!="")echo"<tr><td><input class='input' name='user' autocapitalize='off'>"."<td><input class='input' name='host' value='localhost' autocapitalize='off'>"."<td><input type='submit' class='button' value='".lang(39)."'>\n";echo"</table>\n","</div>\n","</form>\n";}elseif(isset($_GET["sql"])){$O=Admin::get()->getSettings();if($_POST["export"]){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers("sql");Admin::get()->dumpTable("","");Admin::get()->dumpData("","table",$_POST["query"]);exit;}restart_session();$Re=&get_session("queries");$Qe=&$Re[DB];if($_POST["clear"]){$Qe=[];redirect(remove_from_uri("history"));}stop_session();$T=isset($_GET["import"])?lang(74):lang(41);page_header($T,[$T]);$ng="--".(DIALECT=="sql"?" ":"");if($_POST){$oe=false;if(!isset($_GET["import"]))$H=$_POST["query"];elseif($_POST["webfile"]){$df=Admin::get()->getImportFilePath();if($df){if(file_exists($df))$oe=fopen($df,"rb");elseif(file_exists("$df.gz"))$oe=fopen("compress.zlib://$df.gz","rb");}$H=$oe?fread($oe,1e6):false;}else$H=get_file("sql_file",true,";");if(is_string($H)){if(($Og=ini_bytes("memory_limit"))!="-1")ini_set("memory_limit",max($Og,strval(2*strlen($H)+memory_get_usage()+8e6)));if($H!=""&&strlen($H)<1e6){$jj=$H.(preg_match("~;[ \t\r\n]*\$~",$H)?"":";");if(!$Qe||first(end($Qe))!=$jj){restart_session();$Qe[]=[$jj,time()];set_session("queries",$Re);stop_session();}}$Jk="(?:\\s|/\\*[\s\S]*?\\*/|(?:#|$ng)[^\n]*\n?|--\r?\n)";$Ac=";";$Bc=1;$Ch=0;$kd=true;$Vb=connect();if($Vb&&DB!=""){$Vb->selectDatabase(DB);if($_GET["ns"]!="")set_schema($_GET["ns"],$Vb);}$Kb=0;$td=[];$vi='[\'"'.(DIALECT=="sql"?'`#':(DIALECT=="sqlite"?'`[':(DIALECT=="mssql"?'[':''))).']|/\*|'.$ng.'|$'.(DIALECT=="pgsql"?'|\$([a-zA-Z]\w*)?\$':'');$Tl=microtime(true);$bd=Admin::get()->getDumpFormats();unset($bd["sql"]);while($H!=""){if(!$Ch&&preg_match("~^$Jk*+DELIMITER\\s+(\\S+)~i",$H,$y)){$Ac=preg_quote($y[1]);$Bc=strlen($y[1]);$ke=Admin::get()->formatSqlCommandQuery(trim($y[0]));if($ke!="")echo"<pre><code class='jush-".DIALECT."'>$ke</code></pre>\n";$H=substr($H,strlen($y[0]));}elseif(!$Ch&&DIALECT=="pgsql"&&preg_match("~^($Jk*+COPY\\s+)[^;]+\\s+FROM\\s+stdin;~i",$H,$y)){$Ac="\n\\\\\\.\r?\n";$Bc=3;$Ch=strlen($y[0]);}else{preg_match("($Ac\\s*|$vi)",$H,$y,PREG_OFFSET_CAPTURE,$Ch);list($me,$G)=$y[0];if(!$me&&$oe&&!feof($oe))$H
.=fread($oe,1e5);else{if(!$me&&rtrim($H)=="")break;$Ch=$G+strlen($me);if($me&&!preg_match("(^$Ac)",$me)){$ib=Driver::get()->hasCStyleEscapes()||(DIALECT=="pgsql"&&($G>0&&strtolower($H[$G-1])=="e"));$Hi='(';if($me=='/*')$Hi
.='\*/';elseif($me=='[')$Hi
.=']';elseif(preg_match("~^$ng|^#~",$me))$Hi
.="\n";else$Hi
.=preg_quote($me).($ib?"|\\\\.":"");$Hi
.='|$)s';while(preg_match($Hi,$H,$y,PREG_OFFSET_CAPTURE,$Ch)){$Qj=$y[0][0];if(!$Qj&&$oe&&!feof($oe))$H
.=fread($oe,1e5);else{$Ch=$y[0][1]+strlen($Qj);if(!isset($Qj[0])||$Qj[0]!="\\")break;}}}else{$kd=false;$jj=substr($H,0,$G+$Bc);$Kb++;$bj="<pre id='sql-$Kb'><code class='jush-".DIALECT."'>".Admin::get()->formatSqlCommandQuery(trim($jj))."</code></pre>\n";if(DIALECT=="sqlite"&&preg_match("~^$Jk*+(ATTACH|VACUUM\\b.*\\bINTO)\\b~is",$jj,$y)!==0){echo$bj,"<p class='error'>".lang(191,preg_match('~ATTACH~i',$y[1])?'ATTACH':'VACUUM INTO')."\n";$td[]=" <a href='#sql-$Kb'>$Kb</a>";if($_POST["error_stops"])break;}else{if(!$_POST["only_errors"]){echo$bj;ob_flush();flush();}$Pk=microtime(true);if(Connection::get()->multiQuery($jj)&&is_object($Vb)&&preg_match("~^$Jk*+USE\\b~i",$jj))$Vb->query($jj);do{$I=Connection::get()->storeResult();if(Connection::get()->getError()){echo($_POST["only_errors"]?$bj:""),"<p class='error'>",lang(192),(!empty(Connection::get()->getErrno())?" (".Connection::get()->getErrno().")":""),": ",error()."</p>\n";$td[]=" <a href='#sql-$Kb'>$Kb</a>";if($_POST["error_stops"])break
2;}else{$Jl=" <span class='time'>(".format_time($Pk).")</span>";$fd=(strlen($jj)<1000?" <a href='".h(ME)."sql=".urlencode(trim($jj))."'>".icon("edit").lang(39)."</a>":"");$nj=Connection::get()->getQueryInfo();$za=Connection::get()->getAffectedRows();$Pm=($_POST["only_errors"]?null:Driver::get()->warnings());$Rm="warnings-$Kb";$Sm=$Pm?"<a href='#$Rm' class='toggle'>".lang(40).icon_chevron_down()."</a>":null;$Ad=$fi=null;$Bd="explain-$Kb";$Cd=false;$Dd="export-$Kb";$v=0;if(is_object($I)){if(!$_POST["only_errors"])echo"<div class='table-result'>\n";$v=(int)$_POST["limit"];$fi=print_select_result($I,$Vb,[],$v);if(!$_POST["only_errors"]){echo"<p class='links'>";$_h=$I->getRowsCount();echo($_h?($v&&$_h>$v?lang(193,$v):"").lang(194,$_h):""),$Jl,$fd,$Sm;if($Vb&&preg_match("~^($Jk|\\()*+SELECT\\b~i",$jj)&&($Ad=explain($Vb,$jj)))echo"<a href='#$Bd' class='toggle'>Explain".icon_chevron_down()."</a>";$Cd=true;echo"<a href='#$Dd' class='toggle'>".lang(75).icon_chevron_down()."</a>","</p>\n";}}else{if(preg_match("~^$Jk*+(CREATE|DROP|ALTER)$Jk++(DATABASE|SCHEMA)\\b~i",$jj)){restart_session();set_session("dbs",null);stop_session();}if(!$_POST["only_errors"]){echo"<p class='message' title='".h($nj)."'>",lang(195,$za),"$Jl $fd";if($Sm)echo", $Sm";echo"</p>\n";}}if(!$_POST["only_errors"])echo
script("initToggles(qsl('p'));");if($Pm)echo"<div id='$Rm' class='hidden'>\n$Pm</div>\n";if($Ad){echo"<div id='$Bd' class='hidden explain'>\n";print_select_result($Ad,$Vb,$fi);echo"</div>\n";}if($Cd){echo"<form id='$Dd' action='' method='post' class='hidden'><p>\n",html_select("format",$bd,$O->getParameter("exportFormat")),html_select("output",Admin::get()->getDumpOutputs(),$O->getParameter("exportOutput"))." ",input_hidden("query",$jj),input_token()," <input type='submit' class='button' name='export' value='".lang(75)."'>";if(!$v)echo
script("qsl('input').onclick = function (event) { return sqlExport.call(this, event, '".js_escape(ME)."set=export-settings'); };","");echo"</p></form>\n";}if(is_object($I)&&!$_POST["only_errors"])echo"</div>\n";}$Pk=microtime(true);}while(Connection::get()->nextResult());}$H=substr($H,$Ch);$Ch=0;}}}}if($kd)echo"<p class='message'>".lang(196)."\n";elseif($_POST["only_errors"]){$Fh=$Kb-count($td);echo"<p class='".($Fh?"message":"error")."'>".lang(197,$Kb-count($td))," <span class='time'>(".format_time($Tl).")</span>\n";}elseif($td&&$Kb>1)echo"<p class='error'>".lang(192).": ".implode("",$td)."\n";}else
echo"<p class='error'>".upload_error($H)."\n";}echo"<form action='' method='post' enctype='multipart/form-data' id='form'>\n";if(!isset($_GET["import"])){$jj=$_GET["sql"];if($_POST)$jj=$_POST["query"];elseif($_GET["history"]=="all")$jj=$Qe;elseif($_GET["history"]!="")$jj=$Qe[$_GET["history"]][0];echo"<p>";textarea("query",$jj,20);echo
script(($_POST?"":"qs('textarea').focus();\n")."gid('form').onsubmit = partial(sqlSubmit, gid('form'), '".js_escape(remove_from_uri("sql|limit|error_stops|only_errors|history"))."');"),"</p>","<p><input type='submit' class='button default' value='".lang(198)."' title='Ctrl+Enter'>",lang(199).": <input type='number' name='limit' class='input size' value='".h($_POST?$_POST["limit"]:$_GET["limit"])."'>\n";}else{echo"<div class='field-sets'>\n","<fieldset><legend>".lang(200)."</legend><div class='fieldset-content'>";$Ee=(extension_loaded("zlib")?"[.gz]":"");if(ini_bool("file_uploads"))echo"SQL$Ee (&lt; ".ini_get("upload_max_filesize")."B): <input type='file' name='sql_file[]' multiple>","<input type='submit' class='button default' value='".lang(198)."'>",file_upload_form_script("form","sql_file[]");else
echo
lang(201);echo"</div></fieldset>\n";$df=Admin::get()->getImportFilePath();if($df)echo"<fieldset><legend>".lang(202)."</legend><div class='fieldset-content'>",lang(203,"<code>".h($df)."$Ee</code>")," <input type='submit' class='button default' name='webfile' value='".lang(204)."'>","</div></fieldset>\n";echo"</div>\n","<p>";}echo
checkbox("error_stops",1,($_POST?$_POST["error_stops"]:(isset($_GET["error_stops"])?$_GET["error_stops"]:true)),lang(205)),checkbox("only_errors",1,($_POST?$_POST["only_errors"]:isset($_GET["import"])||$_GET["only_errors"]),lang(206)),input_token(),"</p>\n";if(!isset($_GET["import"]))Admin::get()->printAfterSqlCommand();if(!isset($_GET["import"])&&$Qe){echo"<div class='field-sets'>\n";print_fieldset_start("history",lang(207),"history",$_GET["history"]!="");for($X=end($Qe);$X;$X=prev($Qe)){$t=key($Qe);list($jj,$Jl,$jd)=$X;echo" <pre><code class='jush-".DIALECT."'>",truncate_utf8(preg_replace('~\s+~',' ',ltrim(preg_replace("~^(#|$ng).*~m",'',$jj)))),"</code></pre>",'<p class="links">',"<a href='".h(ME."sql=&history=$t")."'>".icon("edit").lang(39)."</a>"," <span class='time' title='".@date('Y-m-d',$Jl)."'>".@date("H:i:s",$Jl).($jd?" ($jd)":"")."</span>","</p>";}echo"<p><input type='submit' class='button' name='clear' value='".lang(208)."'>\n","<a href='",h(ME."sql=&history=all")."' class='button light'>",icon("edit"),lang(209),"</a></p>\n";print_fieldset_end("history");echo"</div>\n";}echo"</form>\n";}elseif(isset($_GET["edit"])){$a=$_GET["edit"];$k=fields($a);$Z=(isset($_GET["select"])?($_POST["check"]&&count($_POST["check"])==1?where_check($_POST["check"][0],$k):""):where($_GET,$k));$pm=(isset($_GET["select"])?$_POST["edit"]:$Z);foreach($k
as$A=>$j){if((!$pm&&!isset($j["privileges"]["insert"]))||Admin::get()->getFieldName($j)=="")unset($k[$A]);}if($_POST&&!isset($_GET["select"])){$x=$_POST["referer"];if($_POST["insert"])$x=($pm?null:$_SERVER["REQUEST_URI"]);elseif(!preg_match('~^.+&select=.+$~',$x))$x=ME."select=".urlencode($a);$s=indexes($a);$jm=unique_array(isset($_GET["where"])?$_GET["where"]:[],$s);$oj="\nWHERE $Z";if(isset($_POST["delete"]))queries_redirect($x,lang(210),(bool)Driver::get()->delete($a,$oj,$jm?0:1));else{$wk=[];foreach($k
as$A=>$j){$X=process_input($j);if($X!==false&&$X!==null)$wk[idf_escape($A)]=$X;}if($pm){if(!$wk)redirect($x);queries_redirect($x,lang(211),(bool)Driver::get()->update($a,$wk,$oj,$jm?0:1));if(is_ajax()){page_headers();page_messages();exit;}}else{$I=Driver::get()->insert($a,$wk);$cg=($I?last_id($I):0);queries_redirect($x,lang(212,($cg?" $cg":"")),(bool)$I);}}}$K=null;if($Z){$M=[];foreach($k
as$A=>$j){if(isset($j["privileges"]["select"])){$La=($_POST["clone"]&&$j["auto_increment"]?"''":convert_field($j));$M[]=($La?"$La AS ":"").idf_escape($A);}}$K=[];if(!support("table"))$M=["*"];if($M){$I=Driver::get()->select($a,$M,[$Z],$M,[],(isset($_GET["select"])?2:1));if(!$I)Admin::get()->addError(error());else{$K=$I->fetchAssoc();if(!$K)$K=false;}if(isset($_GET["select"])&&(!$K||$I->fetchAssoc()))$K=null;}}if(!support("table")&&!$k){if(!$Z){$I=Driver::get()->select($a,["*"],[],["*"]);$K=($I?$I->fetchAssoc():false);if(!$K)$K=[Driver::get()->primary=>""];}if($K){foreach($K
as$t=>$X){if(!$Z)$K[$t]=null;$k[$t]=["field"=>$t,"null"=>($t!=Driver::get()->primary),"auto_increment"=>($t==Driver::get()->primary)];}}}if(isset($_POST["save"])?$_POST["save"]:false){$Ui=[];foreach((isset($_POST["fields"])?$_POST["fields"]:[])as$t=>$X)$Ui[bracket_escape($t,true)]=$X;$K=$Ui+($K?:[]);}if($_POST["edit"]){$hd=array_filter($k,function($j){return!(isset($j["generated"])?$j["generated"]:null);});}else$hd=$k;edit_form($a,$hd,$K,$pm);}elseif(isset($_GET["create"])){$a=$_GET["create"];$zi=Driver::get()->getPartitionBy();$Ci=$zi?Driver::get()->getPartitionsInfo($a):[];$wj=referencable_primary($a);$he=[];foreach($wj
as$ol=>$j)$he[str_replace("`","``",$ol)."`".str_replace("`","``",$j["field"])]=$ol;$ii=[];$R=[];if($a!=""){$ii=fields($a);$R=table_status1($a);if(count($R)<2)Admin::get()->addError(lang(79));}$K=$_POST;$K["Comment"]=normalize_newlines($K["Comment"]);$K["fields"]=(array)$K["fields"];if($K["auto_increment_col"])$K["fields"][$K["auto_increment_col"]]["auto_increment"]=true;if($_POST&&!Admin::get()->getErrors())Admin::get()->getSettings()->updateParameter("commentsOpened",isset($_POST["comments"])?$_POST["comments"]:null);if($_POST&&!process_fields($K["fields"])&&!Admin::get()->getErrors()){if($_POST["drop"])queries_redirect(substr(ME,0,-1),lang(213),drop_tables([$a]));else{$k=[];$Ea=[];$um=false;$fe=[];$hi=reset($ii);$Aa=" FIRST";foreach($K["fields"]as$t=>$j){$n=$he[$j["type"]];$dm=($n!==null?$wj[$n]:$j);if($j["field"]!=""){if(!$j["generated"])$j["default"]=null;$hj=process_field($j,$dm);$Ea[]=[$j["orig"],$hj,$Aa];if(!$hi||$hj!==process_field($hi,$hi)){$k[]=[$j["orig"],$hj,$Aa];if($j["orig"]!=""||$Aa)$um=true;}if($n!==null)$fe[idf_escape($j["field"])]=($a!=""&&DIALECT!="sqlite"?"ADD":" ").format_foreign_key(['table'=>$he[$j["type"]],'source'=>[$j["field"]],'target'=>[$dm["field"]],'on_delete'=>$j["on_delete"],]);$Aa=" AFTER ".idf_escape($j["field"]);}elseif($j["orig"]!=""){$um=true;$k[]=[$j["orig"]];}if($j["orig"]!=""){$hi=next($ii);if(!$hi)$Aa="";}}$Ai=[];if(in_array($K["partition_by"],$zi)){foreach($K
as$t=>$X){if(preg_match('~^partition~',$t))$Ai[$t]=$X;}foreach($Ai["partition_names"]as$t=>$A){if($A===""){unset($Ai["partition_names"][$t]);unset($Ai["partition_values"][$t]);}}$Ai["partition_names"]=array_values($Ai["partition_names"]);$Ai["partition_values"]=array_values($Ai["partition_values"]);if($Ai==$Ci)$Ai=[];}elseif(str_contains(isset($R["Create_options"])?$R["Create_options"]:"","partitioned"))$Ai=null;$_=lang(214);if($a==""){cookie("neo_engine",isset($K["Engine"])?$K["Engine"]:"");$_=lang(215);}$A=trim($K["name"]);$x=ME.(support("table")?"table=":"select=").urlencode($A);$I=alter_table($a,$A,(DIALECT=="sqlite"&&($um||$fe)?$Ea:$k),$fe,($K["Comment"]!=$R["Comment"]?$K["Comment"]:null),($K["Engine"]&&$K["Engine"]!=$R["Engine"]?$K["Engine"]:""),($K["Collation"]&&$K["Collation"]!=$R["Collation"]?$K["Collation"]:""),($K["Auto_increment"]!=""?number($K["Auto_increment"]):""),$Ai);if($I&&!Queries::$queries)redirect($x);queries_redirect($x,$_,$I);}}if($a!="")page_header(lang(36).": ".h($a),["table"=>$a,lang(36)]);else
page_header(lang(78),[lang(78)]);if(!$_POST){$fm=Driver::get()->getTypes();$K=["Engine"=>$_COOKIE["neo_engine"],"fields"=>[["field"=>"","type"=>(isset($fm["int"])?"int":(isset($fm["integer"])?"integer":"")),"on_update"=>""]],"partition_names"=>[""],];if($a!=""){$K=$R;$K["name"]=$a;$K["fields"]=[];if(!$_GET["auto_increment"])$K["Auto_increment"]="";foreach($ii
as$j){$j["generated"]=$j["generated"]?:(isset($j["default"])?"DEFAULT":"");$K["fields"][]=$j;}if($zi){$K+=$Ci;$K["partition_names"][]="";$K["partition_values"][]="";}}}$Nf=[];if($K["Collation"])$Nf[$K["Collation"]]=true;foreach($K["fields"]as$j){if($j["collation"])$Nf[$j["collation"]]=true;}$Cb=Admin::get()->getCollations(array_keys($Nf));$pd=Driver::get()->engines();foreach($pd
as$od){if(!strcasecmp($od,$K["Engine"])){$K["Engine"]=$od;break;}}$Dg=max_input_vars(12,20);if($Dg){$Ne=(count($K["fields"])>$Dg?"":" hidden");echo"<p".($Ne?" id='max-fields' data-columns='$Dg'":"")." class='error$Ne'>".max_input_vars_error()."\n";}echo"<form action='' method='post' id='form'>\n";if(support("columns")||$a==""){echo"<p>",lang(216),": ","<input class='input' name='name' data-maxlength='64' value='",h($K["name"]),"' autocapitalize='off'",(($a==""&&!$_POST)?" autofocus":""),">";if($pd)echo" ",html_select("Engine",[""=>"(".lang(217).")"]+$pd,$K["Engine"]),help_script_command("value",true);if($Cb&&!preg_match("~sqlite|mssql~",DIALECT))echo" ",html_select("Collation",[""=>"(".lang(92).")"]+$Cb,$K["Collation"]);echo" <input type='submit' class='button default' value='",lang(115),"'>","</p>";}if(support("columns")&&($a==""||!Driver::get()->isPartition($a))){echo"<div class='scrollable'>\n","<table id='edit-fields' class='nowrap'>\n";edit_fields($K["fields"],$Cb,"TABLE",$he);echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>",lang(48),": ","<input type='number' class='input size' name='Auto_increment' size='6' value='",h($K["Auto_increment"]),"'>";$Ob=$_POST?$_POST["comments"]:Admin::get()->getSettings()->getParameter("commentsOpened");$Mb=$Ob?"":"hidden";if(support("comment")){echo
checkbox("comments",1,$Ob,lang(47),"editingCommentsClick(this, ".(support("move_col")?7:6).");","jsonly")," ";if(preg_match('~\n~',$K["Comment"]))echo"<textarea name='Comment' rows='2' cols='20'",($Mb?" class='$Mb'":""),">",h($K["Comment"]),"</textarea>";else
echo"<input name='Comment' value='",h($K["Comment"]),"' data-maxlength='",(Connection::get()->isMinVersion("5.5")?2048:60),"' class='input $Mb'>";}echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(115),"'>";}elseif($a!="")echo"<p>";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$a)),"</p>\n";if($zi&&(DIALECT=="sql"||$a=="")){echo"<div class='field-sets'>\n";$_i=preg_match('~RANGE|LIST~',$K["partition_by"]);print_fieldset_start("partition",lang(219),"split",(bool)$K["partition_by"]);echo"<p>",html_select("partition_by",array_merge([""],$zi),$K["partition_by"]),help_script_command("value.replace(/./, 'PARTITION BY \$&')",true),script("qsl('select').onchange = partitionByChange;"),"(<input class='input' name='partition' value='",h($K["partition"]),"'>) ",lang(50),": ","<input type='number' name='partitions' class='input size ",($_i||!$K["partition_by"]?"hidden":""),"' value='",h($K["partitions"]),"'>","</p>\n","<table id='partition-table'",($_i?"":" class='hidden'"),">\n","<thead><tr><th>",lang(220),"</th><th>",lang(52),"</th></tr></thead>\n";foreach($K["partition_names"]as$t=>$X){echo"<tr>","<td><input class='input' name='partition_names[]' value='",h($X),"' autocapitalize='off'>";if($t==count($K["partition_names"])-1)echo
script("qsl('input').oninput = partitionNameChange;");echo"</td>","<td><input class='input' name='partition_values[]' value='",h(isset($K["partition_values"][$t])?$K["partition_values"][$t]:""),"'></td>","</tr>\n";}echo"</table>\n","</p>\n";print_fieldset_end("partition");echo"</div>\n";}echo
input_token(),"</form>\n";}elseif(isset($_GET["indexes"])){$a=$_GET["indexes"];$jf=["PRIMARY","UNIQUE","INDEX"];$R=table_status1($a,true);$hf=Driver::get()->getIndexAlgorithms($R);$d=Connection::get();$_g=$d->isMariaDB();if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($_g?"10.0.5":"5.6")?'|InnoDB':'').'~i',$R["Engine"]))$jf[]="FULLTEXT";if(preg_match('~MyISAM|M?aria'.($d->isMinVersion($_g?"10.2.2":"5.7")?'|InnoDB':'').'~i',$R["Engine"]))$jf[]="SPATIAL";if($_g&&$d->isMinVersion("11.7")&&preg_match('~MyISAM|InnoDB~i',$R["Engine"]))$jf[]="VECTOR";$s=indexes($a);$k=fields($a);$aj=[];if(DIALECT=="mongo"){$aj=$s["_id_"];unset($jf[0]);unset($s["_id_"]);}$K=$_POST;if($K){$O=Admin::get()->getSettings();if($O->getParameter("indexOptions")!==null)$O->updateParameter("indexOptions",null);}if($_POST&&!$_POST["add"]&&!$_POST["drop_col"]){$Ga=[];foreach($K["indexes"]as$r){$A=$r["name"];if(in_array($r["type"],$jf)){$c=[];$kg=[];$Ec=[];$Sh=[];$gf=$hf?(in_array($r["algorithm"],$hf)?$r["algorithm"]:first($hf)):"";$if=(support("partial_indexes")?$r["partial"]:"");$wk=[];ksort($r["columns"]);foreach($r["columns"]as$t=>$b){if($b!=""){$u=isset($r["lengths"][$t])?$r["lengths"][$t]:null;$Cc=isset($r["descs"][$t])?$r["descs"][$t]:null;$Rh=isset($r["opclasses"][$t])?$r["opclasses"][$t]:null;$wk[]=($k[$b]?idf_escape($b):$b).($u?"(".(+$u).")":"").($Rh!=""?" ".idf_escape($Rh):"").($Cc?" DESC":"");$c[]=$b;$kg[]=($u?:null);$Ec[]=$Cc;$Sh[]="$Rh";}}$_d=$s[$A];if($_d){ksort($_d["columns"]);ksort($_d["lengths"]);ksort($_d["descs"]);if($r["type"]==$_d["type"]&&array_values($_d["columns"])===$c&&(!$_d["lengths"]||array_values($_d["lengths"])===$kg)&&array_values($_d["descs"])===$Ec&&(!$_d["opclasses"]||array_values($_d["opclasses"])===$Sh)&&(!$hf||$_d["algorithm"]===$gf)&&$_d["partial"]==$if){unset($s[$A]);continue;}}if($c)$Ga[]=[$r["type"],$A,$wk,$gf,$if];}}foreach($s
as$A=>$_d)$Ga[]=[$_d["type"],$A,"DROP"];if(!$Ga)redirect(ME."table=".urlencode($a));queries_redirect(ME."table=".urlencode($a),lang(221),alter_indexes($a,$Ga));}page_header(lang(173).": ".h($a),["table"=>$a,lang(173)]);$Sd=array_keys($k);if($_POST["add"]){foreach($K["indexes"]as$t=>$r){if($r["columns"][count($r["columns"])]!="")$K["indexes"][$t]["columns"][]="";}$r=end($K["indexes"]);if($r["type"]||array_filter($r["columns"],'strlen'))$K["indexes"][]=["columns"=>[1=>""]];}if(!$K){foreach($s
as$t=>$r){$s[$t]["name"]=$t;$s[$t]["columns"][]="";}$s[]=["columns"=>[1=>""]];$K["indexes"]=$s;}$kg=(DIALECT=="sql"||DIALECT=="mssql");$Sh=Driver::get()->getIndexOpclasses();if($_POST)$_k=$_POST["options"];else{$_k=false;foreach($s
as$r){if(array_filter(isset($r["lengths"])?$r["lengths"]:[])||array_filter(isset($r["descs"])?$r["descs"]:[])||array_filter(isset($r["opclasses"])?$r["opclasses"]:[])||(isset($r["partial"])?$r["partial"]:"")!=""){$_k=true;break;}}}echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap'>\n","<thead><tr>","<th id='label-type'>",lang(222),"</th>";$Xh="class='idxopts".($_k?"":" hidden")."'";if(count($hf)>1)echo"<th id='label-method' $Xh>",lang(223),doc_link(['sql'=>'create-index.html#create-index-storage-engine-index-types','mariadb'=>'ha-and-performance/optimization-and-tuning/optimization-and-indexes/storage-engine-index-types',]),"</th>";echo"<th><input type='submit' hidden>",lang(53).($kg?"<span $Xh> (".lang(54).")</span>":"");if($kg||support("descidx"))echo
checkbox("options",1,$_k,lang(98),"indexOptionsShow(this.checked)","jsonly")."\n";echo"</th>","<th id='label-name'>",lang(224),"</th>";if(support("partial_indexes"))echo"<th id='label-condition' $Xh>",lang(55),"</th>";echo"<th>","<button name='add[0]' value='1' title='",lang(99),"' class='button light hidden'>",icon_solo("add"),"</button>","</th>","</tr></thead>\n";if($aj){echo"<tr><td>PRIMARY<td>";foreach($aj["columns"]as$b)echo
select_input(" disabled",array_combine($Sd,$Sd),$b),"<label><input type='checkbox' disabled>".lang(63)."</label> ";echo"<td><td>\n";}$If=1;foreach($K["indexes"]as$r){if(!$_POST["drop_col"]||$If!=key($_POST["drop_col"])){echo"<tr><td>",html_select("indexes[$If][type]",[-1=>""]+$jf,$r["type"],($If==count($K["indexes"])?"indexesAddRow.call(this);":""),"label-type"),"</td>";if(count($hf)>1)echo"<td $Xh>",html_select("indexes[$If][algorithm]",array_merge([""],$hf),$r['algorithm'],"label-method"),"</td>";echo"<td>";ksort($r["columns"]);$p=1;foreach($r["columns"]as$t=>$b){echo"<span>".select_input(" name='indexes[$If][columns][$p]' title='".lang(44)."'",($k&&($b==""||$k[$b])?array_combine($Sd,$Sd):[]),$b,"partial(indexesChangeColumn, '".js_escape(DIALECT=="sql"?"":$_GET["indexes"]."_")."')"),"<span $Xh>";if($kg)echo"<input type='number' name='indexes[$If][lengths][$p]' class='input size' value='".(h(isset($r["lengths"][$t])?$r["lengths"][$t]:"")),"' title='".lang(97),"'>";if($Sh){$Rh=isset($r["opclasses"][$t])?$r["opclasses"][$t]:"";echo
html_select("indexes[$If][opclasses][$p]",[""=>"(".lang(225).")"]+array_combine($Sh,$Sh)+($Rh!=""?[$Rh=>$Rh]:[]),$Rh),'';}if(support("descidx"))echo
checkbox("indexes[$If][descs][$p]",1,isset($r["descs"][$t])?$r["descs"][$t]:false,lang(63));echo"<br></span></span>";$p++;}echo"</td>","<td><input name='indexes[$If][name]' value='",h($r["name"]),"' class='input' autocapitalize='off' aria-labelledby='label-name'></td>\n";if(support("partial_indexes"))echo"<td $Xh><input name='indexes[$If][partial]' value='".h($r["partial"])."' autocapitalize='off' aria-labelledby='label-condition'>\n";echo"<td>","<button name='drop_col[$If]' value='1' title='",lang(59),"' class='button light'>",icon_solo("remove"),"</button>",script("qsl('button').onclick = onRemoveIndexRowClick;"),"</td>\n";}$If++;}echo"</table>\n","</div>\n","<p>","<input type='submit' class='button default' value='",lang(115),"'>",input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["database"])){$K=$_POST;if($_POST&&!isset($_POST["add_x"])){$A=trim($K["name"]);if($_POST["drop"]){$_GET["db"]="";queries_redirect(remove_from_uri("db|database"),lang(226),drop_databases([DB]));}elseif(DB!==$A){if(DB!=""){$_GET["db"]=$A;queries_redirect(preg_replace('~\bdb=[^&]*&~','',ME)."db=".urlencode($A),lang(227),rename_database($A,$K["collation"]));}else{$f=explode("\n",str_replace("\r","",$A));$Yk=true;$bg="";foreach($f
as$g){if(count($f)==1||$g!=""){if(!create_database($g,$K["collation"]))$Yk=false;$bg=$g;}}restart_session();set_session("dbs",null);queries_redirect(ME."db=".urlencode($bg),lang(228),$Yk);}}else{if(!$K["collation"])redirect(substr(ME,0,-1));query_redirect("ALTER DATABASE ".idf_escape($A).(preg_match('~^[a-z0-9_]+$~i',$K["collation"])?" COLLATE $K[collation]":""),substr(ME,0,-1),lang(229));}}if(DB!="")page_header(lang(70).": ".h(DB),[lang(70)]);else
page_header(lang(76),[lang(76)]);$A=DB;if($_POST)$A=$K["name"];elseif(DB!="")$K["collation"]=db_collation(DB,collations());elseif(DIALECT=="sql"){foreach(get_vals("SHOW GRANTS")as$xe){if(preg_match('~ ON (`(([^\\\\`]|``|\\\\.)*)%`\.\*)?~',$xe,$y)&&$y[1]){$A=stripcslashes(idf_unescape("`$y[2]`"));break;}}}$Cb=Admin::get()->getCollations($K["collation"]?[$K["collation"]]:[]);echo"<form action='' method='post'>\n","<p>";if($_POST["add_x"]||strpos($A,"\n"))echo"<textarea id='name' name='name' rows='10' cols='40'>",h($A),"</textarea><br>\n";else
echo"<input class='input' name='name' id='name' value='",h($A),"' data-maxlength='64' autocapitalize='off' autofocus>\n";if($Cb)echo
html_select("collation",[""=>"(".lang(92).")"]+$Cb,$K["collation"]),doc_link(['sql'=>"charset-charsets.html",'mariadb'=>"reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations",]),"\n";echo"<input type='submit' class='button default' value='",lang(115),"'>\n";if(DB!="")echo"<input type='submit' class='button' name='drop' value='".lang(166)."'>".confirm(lang(218,DB))."\n";elseif(!$_POST["add_x"]&&$_GET["db"]=="")echo"<button name='add_x' value='1' title='",lang(99),"' class='button light'>",icon_solo("add"),"</button>\n";echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["call"])){$oa=$_GET["name"]?:$_GET["call"];page_header(lang(230).": ".h($oa),[lang(230)]);$Mj=routine($_GET["call"],(isset($_GET["callf"])?"FUNCTION":"PROCEDURE"));$ef=[];$ni=[];foreach($Mj["fields"]as$p=>$j){if(substr($j["inout"],-3)=="OUT"&&DIALECT=='sql')$ni[$p]="@".idf_escape($j["field"])." AS ".idf_escape($j["field"]);if(!$j["inout"]||substr($j["inout"],0,2)=="IN")$ef[]=$p;}if($_POST){$kb=[];foreach($Mj["fields"]as$t=>$j){$X="";if(in_array($t,$ef)){$X=process_input($j);if($X===false)$X="''";if(isset($ni[$t]))Connection::get()->query("SET @".idf_escape($j["field"])." = $X");}if(isset($ni[$t]))$kb[]="@".idf_escape($j["field"]);elseif(in_array($t,$ef))$kb[]=$X;}$H=(isset($_GET["callf"])?"SELECT ":"CALL ").($Mj["returns"]&&$Mj["returns"]["type"]=="record"?"* FROM ":"").table($oa)."(".implode(", ",$kb).")";$Pk=microtime(true);$I=Connection::get()->multiQuery($H);$za=Connection::get()->getAffectedRows();echo
Admin::get()->formatSelectQuery($H,$Pk,!$I);if(!$I)echo"<p class='error'>".error()."\n";else{$Vb=connect();if($Vb)$Vb->selectDatabase(DB);do{$I=Connection::get()->storeResult();if(is_object($I))print_select_result($I,$Vb);else
echo"<p class='message'>".lang(231,$za)." <span class='time'>".@date("H:i:s")."</span>\n";}while(Connection::get()->nextResult());if($ni)print_select_result(Connection::get()->query("SELECT ".implode(", ",$ni)));}}echo"<form action='' method='post'>\n";if($ef){echo"<table class='box'>\n";foreach($ef
as$t){$j=$Mj["fields"][$t];$A=$j["field"];echo"<tr><th>".Admin::get()->getFieldName($j);$Y=isset($_POST["fields"][$A])?$_POST["fields"][$A]:"";if($Y!=""){if($j["type"]=="set")$Y=implode(",",$Y);}input($j,$Y,(string)(isset($_POST["function"][$A])?$_POST["function"][$A]:""));echo"\n";}echo"</table>\n";}echo"<p>\n","<input type='submit' class='button' value='",lang(230),"'>\n",input_token(),"</p>\n","</form>\n";$Lb=$Mj["comment"];if($Lb!==null&&$Lb!==""){$Lb=h(trim($Mj["comment"],"\n"));if(preg_match('~^ +~',$Lb,$z)){preg_match_all("~^($z[0]|$)~m",$Lb,$pg);if(count($pg[0])==substr_count($Lb,"\n"))$Lb=preg_replace("~^($z[0])~m","",$Lb);}$Lb=preg_replace('~(^|[^\n]\n)(Description|Parameters|Example)\n~',"$1\n<strong>$2</strong>\n",$Lb);echo"<pre class='comment'>$Lb</pre>\n";}}elseif(isset($_GET["foreign"])){$a=$_GET["foreign"];$A=$_GET["name"];$K=$_POST;if($_POST&&!$_POST["add"]&&!$_POST["change"]&&!$_POST["change-js"]){if(!$_POST["drop"]){$K["source"]=array_filter($K["source"],'strlen');ksort($K["source"]);$_l=[];foreach($K["source"]as$t=>$X)$_l[$t]=$K["target"][$t];$K["target"]=$_l;}if(DIALECT=="sqlite")$I=recreate_table($a,$a,[],[],[" $A"=>($K["drop"]?"":" ".format_foreign_key($K))]);else{$Ga="ALTER TABLE ".table($a);$I=($A==""||queries("$Ga DROP ".(DIALECT=="sql"?"FOREIGN KEY ":"CONSTRAINT ").idf_escape($A)));if(!$K["drop"])$I=queries("$Ga ADD".format_foreign_key($K));}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(232):($A!=""?lang(233):lang(234))),(bool)$I);if(!$K["drop"])Admin::get()->addError(lang(235));}if($A!="")page_header(lang(236).": ".h($A),["table"=>$a,lang(236)]);else
page_header(lang(177).": ".h($a),["table"=>$a,lang(177)]);if($_POST){ksort($K["source"]);if($_POST["change"]||$_POST["change-js"])$K["target"]=[];else$K["source"][]="";}elseif($A!=""){$he=foreign_keys($a);$K=$he[$A];$K["source"][]="";}else{$K["table"]=$a;$K["source"]=[""];}echo"<form action='' method='post'>\n";$Hk=array_keys(fields($a));if($K["db"]!="")Connection::get()->selectDatabase($K["db"]);if($K["ns"]!=""){$ji=get_schema();set_schema($K["ns"]);}$vj=array_keys(array_filter(table_status('',true),'AdminNeo\fk_support'));$_l=$vj?array_keys(fields(in_array($K["table"],$vj)?$K["table"]:reset($vj))):[];$Nh="this.form['change-js'].value = '1'; this.form.submit();";echo"<p>","<span id='label-table'>",lang(237),":</span> ",html_select("table",$vj,$K["table"],$Nh,"label-table");if(DIALECT!="sqlite"){$tc=[];foreach(Admin::get()->getDatabases()as$g){if(!information_schema($g))$tc[]=$g;}echo"<span id='label-db'>",lang(238),":</span> ",html_select("db",$tc,$K["db"]!=""?$K["db"]:$_GET["db"],$Nh,"label-db");}echo
input_hidden("change-js"),"<noscript><input type='submit' class='button' name='change' value='",lang(239),"'></noscript>","</p>\n","<table>","<thead><tr><th id='label-source'>",lang(174),"<th id='label-target'>",lang(175),"</thead>\n";$If=0;foreach($K["source"]as$t=>$X){echo"<tr>","<td>".html_select("source[".(+$t)."]",[-1=>""]+$Hk,$X,($If==count($K["source"])-1?"foreignAddRow.call(this);":""),"label-source"),"<td>".html_select("target[".(+$t)."]",$_l,isset($K["target"][$t])?$K["target"][$t]:null,"","label-target");$If++;}echo"</table>\n","<noscript><p><input type='submit' class='button' name='add' value='",lang(240),"'></p></noscript>","<p>\n","<span id='label-delete'>".lang(94),":</span> ",html_select("on_delete",[-1=>""]+Driver::get()->getOnActions(),$K["on_delete"],"","label-delete"),"<span id='label-update'>".lang(93),":</span> ",html_select("on_update",[-1=>""]+Driver::get()->getOnActions(),$K["on_update"],"","label-update");if(DRIVER=='pgsql')echo
html_select("deferrable",['NOT DEFERRABLE','DEFERRABLE','DEFERRABLE INITIALLY DEFERRED'],$K["deferrable"]);echo
doc_link(['sql'=>"innodb-foreign-key-constraints.html",'mariadb'=>"architecture/server-constraints/foreign-key-constraints",]),"</p>\n<p>","<input type='submit' class='button default' value='",lang(115),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["view"])){$a=$_GET["view"];$K=$_POST;$ki="VIEW";if(DIALECT=="pgsql"&&$a!=""){$P=table_status1($a);$ki=strtoupper($P["Engine"]);}if($_POST){$A=trim($K["name"]);$La=" AS\n$K[select]";$x=ME."table=".urlencode($A);$_=lang(241);$U=($_POST["materialized"]?"MATERIALIZED VIEW":"VIEW");if(!$_POST["drop"]&&$a==$A&&DIALECT!="sqlite"&&$U=="VIEW"&&$ki=="VIEW")query_redirect((DIALECT=="mssql"?"ALTER":"CREATE OR REPLACE")." VIEW ".table($A).$La,$x,$_);else{$Bl=$A."_adminneo_".uniqid();drop_create("DROP $ki ".table($a),"CREATE $U ".table($A).$La,"DROP $U ".table($A),"CREATE $U ".table($Bl).$La,"DROP $U ".table($Bl),($_POST["drop"]?substr(ME,0,-1):$x),lang(242),$_,lang(243),$a,$A);}}if(!$_POST&&$a!=""){$K=view($a);$K["name"]=$a;$K["materialized"]=($ki!="VIEW");if($i=error())Admin::get()->addError($i);}if($a!="")page_header(lang(37).": ".h($a),["table"=>$a,lang(37)]);else
page_header(lang(244),[lang(244)]);echo"<form action='' method='post'>\n","<p>",lang(224),":","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>\n";if(support("materializedview"))echo
checkbox("materialized",1,$K["materialized"],lang(167));echo"</p>\n<p>";textarea("select",$K["select"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(115),"'>\n";if($a!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>\n",confirm(lang(218,$a));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["event"])){$ea=$_GET["event"];$vf=["YEAR","QUARTER","MONTH","DAY","HOUR","MINUTE","WEEK","SECOND","YEAR_MONTH","DAY_HOUR","DAY_MINUTE","DAY_SECOND","HOUR_MINUTE","HOUR_SECOND","MINUTE_SECOND"];$Rk=["ENABLED"=>"ENABLE","DISABLED"=>"DISABLE","SLAVESIDE_DISABLED"=>"DISABLE ON SLAVE"];$K=$_POST;if($_POST){if($_POST["drop"])query_redirect("DROP EVENT ".idf_escape($ea),substr(ME,0,-1),lang(245));elseif(in_array($K["INTERVAL_FIELD"],$vf)&&isset($Rk[$K["STATUS"]])){$Xj="\nON SCHEDULE ".($K["INTERVAL_VALUE"]?"EVERY ".q($K["INTERVAL_VALUE"])." $K[INTERVAL_FIELD]".($K["STARTS"]?" STARTS ".q($K["STARTS"]):"").($K["ENDS"]?" ENDS ".q($K["ENDS"]):""):"AT ".q($K["STARTS"]))." ON COMPLETION".($K["ON_COMPLETION"]?"":" NOT")." PRESERVE";queries_redirect(substr(ME,0,-1),($ea!=""?lang(246):lang(247)),(bool)queries(($ea!=""?"ALTER EVENT ".idf_escape($ea).$Xj.($ea!=$K["EVENT_NAME"]?"\nRENAME TO ".idf_escape($K["EVENT_NAME"]):""):"CREATE EVENT ".idf_escape($K["EVENT_NAME"]).$Xj)."\n".$Rk[$K["STATUS"]]." COMMENT ".q($K["EVENT_COMMENT"]).rtrim(" DO\n$K[EVENT_DEFINITION]",";").";"));}}if($ea!="")page_header(lang(248).": ".h($ea),[lang(248)]);else
page_header(lang(249),[lang(249)]);if(!$K&&$ea!=""){$L=get_rows("SELECT * FROM information_schema.EVENTS WHERE EVENT_SCHEMA = ".q(DB)." AND EVENT_NAME = ".q($ea));$K=reset($L);}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(224),"</th><td>","<input class='input' name='EVENT_NAME' value='",h($K["EVENT_NAME"]),"' data-maxlength='64' autocapitalize='off'>","</td></tr>\n","<tr><th title='datetime'>",lang(250),"</th><td>","<input class='input' name='STARTS' value='",h("$K[EXECUTE_AT]$K[STARTS]"),"'>","</td></tr>\n","<tr><th title='datetime'>",lang(251),"</th><td>","<input class='input' name='ENDS' value='",h($K["ENDS"]),"'>","</td></tr>\n","<tr><th>",lang(252),"</th><td>","<input type='number' name='INTERVAL_VALUE' value='",h($K["INTERVAL_VALUE"]),"' class='input size'> ",html_select("INTERVAL_FIELD",$vf,$K["INTERVAL_FIELD"]),"</td></tr>\n","<tr><th>",lang(158),"</th><td>",html_select("STATUS",$Rk,$K["STATUS"]),"</td></tr>\n","<tr><th>",lang(47),"</th><td>","<input class='input' name='EVENT_COMMENT' value='",h($K["EVENT_COMMENT"]),"' data-maxlength='64'>","</td></tr>\n","<tr><th></th><td>",checkbox("ON_COMPLETION","PRESERVE",$K["ON_COMPLETION"]=="PRESERVE",lang(253)),"</td></tr>\n","</table>\n","<p>";textarea("EVENT_DEFINITION",$K["EVENT_DEFINITION"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(115),"'>";if($ea!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$ea));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["procedure"])){$oa=($_GET["name"]?:$_GET["procedure"]);$Mj=(isset($_GET["function"])?"FUNCTION":"PROCEDURE");$K=$_POST;$K["fields"]=(array)$K["fields"];if($_POST&&!process_fields($K["fields"])){foreach($K["fields"]as$t=>$j){if($j["field"]=="")unset($K["fields"][$t]);}$Ih=routine_id($oa,routine($_GET["procedure"],$Mj));$sh=routine_id($K["name"],$K);$ec=create_routine($Mj,$K);$x=substr(ME,0,-1);$_=lang(254);if(!$_POST["drop"]&&$Ih==$sh&&(DIALECT!="sql"||Connection::get()->isMariaDB()))query_redirect(substr_replace($ec,' OR REPLACE',6,0),$x,$_);else{$Bl="$K[name]_adminer_".uniqid();drop_create("DROP $Mj $Ih",$ec,"DROP $Mj $sh",create_routine($Mj,["name"=>$Bl]+$K),"DROP $Mj ".routine_id($Bl,$K),$x,lang(255),$_,lang(256),$oa,$K["name"]);}}if($oa!=""){$T=isset($_GET["function"])?lang(257):lang(258);page_header($T.": ".h($oa),[$T]);}else{$T=isset($_GET["function"])?lang(259):lang(260);page_header($T,[$T]);}if(!$_POST){if($oa=="")$K["language"]="sql";else{$K=routine($_GET["procedure"],$Mj);$K["name"]=$oa;}}$pb=get_vals("SHOW CHARACTER SET");sort($pb);$Nj=routine_languages();echo"<form action='' method='post' id='form'>\n","<p>",lang(224),": ","<input class='input' name='name' value='",h($K["name"]),"' data-maxlength='64' autocapitalize='off'>";if($Nj)echo"<span id='label-language'>",lang(10),":</span> ",html_select("language",$Nj,$K["language"],"","label-language");echo"<input type='submit' class='button default' value='",lang(115),"'>","</p>\n","<div class='scrollable'>\n","<table class='nowrap' id='edit-fields'>\n";edit_fields($K["fields"],$pb,$Mj);if(isset($_GET["function"])){echo"<tbody><tr>";if(support("move_col"))echo"<th></th>";echo"<th>",lang(261),"</th>";edit_type("returns",(array)$K["returns"],$pb,[],(DIALECT=="pgsql"?["void","trigger"]:[]));echo"<td></td>","</tr></tbody>\n";}echo"</table>\n",script("initFieldsEditing(gid('edit-fields'));");if(support("move_col"))echo
script("initSortable('#edit-fields tbody');");echo"</div>\n","<p>";textarea("definition",$K["definition"],20);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(115),"'>";if($oa!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$oa));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["check"])){$a=$_GET["check"];$A=$_GET["name"];$K=$_POST;if($K){if(DIALECT=="sqlite")$Yk=recreate_table($a,$a,[],[],[],"",[],"$A",($K["drop"]?"":$K["clause"]));else{$Yk=($A==""||queries("ALTER TABLE ".table($a)." DROP CONSTRAINT ".idf_escape($A)));if(!$K["drop"])$Yk=(bool)queries("ALTER TABLE ".table($a)." ADD".($K["name"]!=""?" CONSTRAINT ".idf_escape($K["name"]):"")." CHECK ($K[clause])");}queries_redirect(ME."table=".urlencode($a),($K["drop"]?lang(262):($A!=""?lang(263):lang(264))),$Yk);}if($A!="")page_header(lang(265).": ".h($A),["table"=>$a,lang(265)]);else
page_header(lang(179).": ".h($a),["table"=>$a,lang(179)]);if(!$K){$ub=Driver::get()->checkConstraints($a);$K=["name"=>$A,"clause"=>$ub[$A]];}echo"<form action='' method='post'>\n","<p>";if(DIALECT!="sqlite")echo
lang(224).': <input name="name" value="'.h($K["name"]).'" class="input" data-maxlength="64" autocapitalize="off"> ';echo
doc_link(['sql'=>"create-table-check-constraints.html",'mariadb'=>"reference/sql-statements/data-definition/constraint",],"?"),"</p>\n<p>";textarea("clause",$K["clause"]);echo"</p>\n<p>","<input type='submit' class='button default' value='",lang(115),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$A));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["trigger"])){$a=$_GET["trigger"];$A=isset($_GET["name"])?$_GET["name"]:"";$Zl=trigger_options();$K=trigger($A,$a)+["Trigger"=>$a."_bi"];if($_POST){if(in_array($_POST["Timing"],$Zl["Timing"])&&in_array($_POST["Event"],$Zl["Event"])&&in_array($_POST["Type"],$Zl["Type"])){$Lh=" ON ".table($a);$Wc="DROP TRIGGER ".idf_escape($A).(DIALECT=="pgsql"?$Lh:"");$x=ME."table=".urlencode($a);if($_POST["drop"])query_redirect($Wc,$x,lang(266));else{if($A!="")queries($Wc);queries_redirect($x,($A!=""?lang(267):lang(268)),(bool)queries(create_trigger($Lh,$_POST)));if($A!="")queries(create_trigger($Lh,$K+["Type"=>reset($Zl["Type"])]));}}$K=$_POST;}if($A!="")page_header(lang(269).": ".h($A),["table"=>$a,lang(269)]);else
page_header(lang(181).": ".h($a),["table"=>$a,lang(181)]);echo"<form action='' method='post' id='form'>\n","<table class='box box-light'>\n","<tr><th id='label-time'>",lang(270),"</th><td>",html_select("Timing",$Zl["Timing"],$K["Timing"],"triggerChange(/^".js_escape_re($a)."_[ba][iud]$/, '".js_escape($a)."', this.form);","label-time"),"</td></tr>\n","<tr><th id='label-event'>",lang(271),"</th><td>",html_select("Event",$Zl["Event"],$K["Event"],"this.form['Timing'].onchange();","label-event");if(in_array("UPDATE OF",$Zl["Event"]))echo" <input name='Of' value='".h($K["Of"])."' class='input hidden'>";echo"</td></tr>\n","<tr><th id='label-type'>",lang(45),"</th><td>",html_select("Type",$Zl["Type"],$K["Type"],"","label-type"),"</td></tr>\n","</table>\n","<p>",lang(224),"<input class='input' name='Trigger' value='",h($K["Trigger"]),"' data-maxlength='64' autocapitalize='off'>","</p>\n",script("gid('form')['Timing'].onchange();"),"<p>";textarea("Statement",$K["Statement"]);echo"</p>\n","<p>","<input type='submit' class='button default' value='",lang(115),"'>";if($A!="")echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>",confirm(lang(218,$A));echo"</p>\n",input_token(),"</form>\n";}elseif(isset($_GET["user"])){$qa=$_GET["user"];$ej=[""=>["All privileges"=>""]];foreach(get_rows("SHOW PRIVILEGES")as$K){foreach(explode(",",($K["Privilege"]=="Grant option"?"":$K["Context"]))as$Zb)$ej[$Zb=="File access on server"?"Server Admin":$Zb][$K["Privilege"]]=$K["Comment"];}unset($ej["Server Admin"]["Usage"]);foreach($ej["Tables"]as$t=>$X)unset($ej["Databases"][$t]);$rh=[];if($_POST){foreach($_POST["objects"]as$t=>$X)$rh[$X]=(array)$rh[$X]+(array)$_POST["grants"][$t];}$ze=[];if(isset($_GET["host"])&&($I=Connection::get()->query("SHOW GRANTS FOR ".q($qa)."@".q($_GET["host"])))){while($K=$I->fetchRow()){if(preg_match('~GRANT (.*) ON (.*) TO ~',$K[0],$y)&&preg_match_all('~ *([^(,]*[^ ,(])( *\([^)]+\))?~',$y[1],$z,PREG_SET_ORDER)){foreach($z
as$X){if($X[1]!="USAGE")$ze["$y[2]$X[2]"][$X[1]]=true;if(preg_match('~ WITH GRANT OPTION~',$K[0]))$ze["$y[2]$X[2]"]["GRANT OPTION"]=true;}}}}$Li=!Connection::get()->isMariaDB()&&Connection::get()->isMinVersion("8");if($_POST){$Kh=(isset($_GET["host"])?q($qa)."@".q($_GET["host"]):"''");if($_POST["drop"])query_redirect("DROP USER $Kh",ME."privileges=",lang(272));else{$uh=q($_POST["user"])."@".q($_POST["host"]);$Ei=$_POST["pass"];$hc=false;$I=true;if($Kh!=$uh){$hc=(bool)queries("CREATE USER $uh IDENTIFIED BY ".($_POST["hashed"]?"PASSWORD ":"").q($Ei));$I=$hc;}elseif($Ei!="")$I=(bool)queries("SET PASSWORD FOR $uh = ".($Li||$_POST["hashed"]?q($Ei):"PASSWORD(".q($Ei).")"));if($I){$Jj=[];foreach($rh
as$Bh=>$xe){if(isset($_GET["grant"]))$xe=array_filter($xe);$xe=array_keys($xe);if(isset($_GET["grant"]))$Jj=array_diff(array_keys(array_filter($rh[$Bh],'strlen')),$xe);elseif($Kh==$uh){$Hh=array_keys((array)$ze[$Bh]);$Jj=array_diff($Hh,$xe);$xe=array_diff($xe,$Hh);unset($ze[$Bh]);}if(preg_match('~^(.+)\s*(\(.*\))?$~U',$Bh,$y)&&(!grant(false,$Jj,$y[2],$y[1],$uh)||!grant(true,$xe,$y[2],$y[1],$uh))){$I=false;break;}}}if($I&&isset($_GET["host"])){if($Kh!=$uh)queries("DROP USER $Kh");elseif(!isset($_GET["grant"])){foreach($ze
as$Bh=>$Jj){if(preg_match('~^(.+)(\(.*\))?$~U',$Bh,$y))grant(false,array_keys($Jj),$y[2],$y[1],$uh);}}}if($I&&!Queries::$queries)redirect(ME."privileges=");queries_redirect(ME."privileges=",(isset($_GET["host"])?lang(273):lang(274)),$I);if($hc)Connection::get()->query("DROP USER $uh");}}$T=isset($_GET["host"])?lang(6).": ".h("$qa@$_GET[host]"):lang(190);$Ml=isset($_GET["host"])?h($qa):lang(190);page_header($T,["privileges"=>['',lang(73)],$Ml]);if($_POST){$K=$_POST;$ze=$rh;}else{$K=$_GET+["host"=>Connection::get()->getValue("SELECT SUBSTRING_INDEX(CURRENT_USER, '@', -1)")];if($ze)$ze[".*"]=[];elseif(DB!="")$ze[idf_escape(addcslashes(DB,"%_\\")).".*"]=[];else$ze["*.* "]=[];}echo"<form action='' method='post'>\n","<table class='box box-light'>\n","<tr><th>",lang(5),"</th>","<td><input class='input' name='host' data-maxlength='60' value='",h($K["host"]),"' autocapitalize='off'></td>\n","<tr><th>",lang(6),"</th>","<td><input class='input' name='user' data-maxlength='80' value='",h($K["user"]),"' autocapitalize='off'></td>\n",'<tr><th>',lang(30),"</th>","<td><input class='input' name='pass' id='pass' value='",h($K["pass"]),"' autocomplete='new-password'>";if(!$Li)echo
checkbox("hashed",1,$K["hashed"],lang(275),"typePassword(this.form['pass'], this.checked);");echo"</td>\n";if(!$K["hashed"])echo
script("typePassword(gid('pass'));");echo"</table>\n","<div class='scrollable'><table class='checkable'>\n","<thead><tr><th colspan='2'>".lang(73).doc_link(['sql'=>"grant.html#priv_level","mariadb"=>"reference/sql-statements/account-management-sql-statements/grant#privilege-levels"])."</th>";$p=0;foreach($ze
as$Bh=>$xe){echo"<th>";if($Bh=="*.*")echo"*.*",input_hidden("objects[$p]","*.*");else
echo"<input class='input' name='objects[$p]' value='".h(trim($Bh))."' size='10' autocapitalize='off'>";echo"</th>";$p++;}echo"</tr></thead>\n";foreach([""=>"","Server Admin"=>lang(5),"Databases"=>lang(31),"Tables"=>lang(9),"Procedures"=>lang(276),]as$Zb=>$Cc){foreach((array)$ej[$Zb]as$dj=>$Lb){echo"<tr>";if($Cc)echo"<td>$Cc</td>";echo"<td".(!$Cc?" colspan='2'":"").' lang="en" title="'.h($Lb).'">'.h($dj)."</td>";$p=0;foreach($ze
as$Bh=>$xe){$A="'grants[$p][".h(strtoupper($dj))."]'";$Y=$xe[strtoupper($dj)];$ij=strpos($Bh,"@")!==false;$qh=$Bh==".*";$Ca=$dj=="All privileges";$ye=$dj=="Grant option";if($Bh=="*.*"&&$dj=="Proxy")echo"<td></td>";elseif($ij&&$dj!="Proxy"&&!$ye)echo"<td></td>";elseif($Zb=="Server Admin"&&$Bh!=(isset($ze["*.*"])?"*.*":".*")&&!(($ij||$qh)&&$dj=="Proxy"))echo"<td></td>";elseif(isset($_GET["grant"]))echo"<td><select name=$A>"."<option></option>"."<option value='1'".($Y?" selected":"").">".lang(277)."</option>"."<option value='0'".($Y=="0"?" selected":"").">".lang(278)."</option>"."</select></td>";else{echo"<td class='center'><label class='block'>","<input type='checkbox' name=$A value='1'".($Y?" checked":"").($Ca?" id='grants-$p-all'":(!$ye?" class='grants-$p'":"")).">";if($Ca)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheckAll('.grants-$p'); };");elseif(!$ye)echo
script("qsl('input').onclick = function () { if (this.checked) formUncheck('grants-$p-all'); };");echo"</label>";}$p++;}echo"</tr>";}}echo"</table></div>\n","<p>","<input type='submit' class='button default' value='",lang(115),"'>\n";if(isset($_GET["host"]))echo"<input type='submit' class='button' name='drop' value='",lang(166),"'>\n",confirm(lang(218,"$qa@$_GET[host]"));echo
input_token(),"</p>\n","</form>\n";}elseif(isset($_GET["processlist"])){if(support("kill")){if($_POST){$Tf=0;foreach((array)$_POST["kill"]as$X){if(kill_process($X))$Tf++;}queries_redirect(ME."processlist=",lang(279,$Tf),$Tf||!$_POST["kill"]);}}page_header(lang(156),[lang(156)]);echo"<form action='' method='post'>\n","<div class='scrollable'>\n","<table class='nowrap checkable'>\n";$p=-1;foreach(process_list()as$p=>$K){if(!$p){echo"<thead><tr lang='en'>".(support("kill")?"<th>":"");foreach($K
as$t=>$X)echo"<th>$t".doc_link(['sql'=>"show-processlist.html#processlist_".strtolower($t),'mariadb'=>"reference/sql-statements/administrative-sql-statements/show/show-processlist",]);echo"</thead>\n","<tbody>\n";}echo"<tr>".(support("kill")?"<td>".checkbox("kill[]",$K[DIALECT=="sql"?"Id":"pid"],0):"");foreach($K
as$t=>$X)echo"<td>".($X!=""&&((DIALECT=="sql"&&$t=="Info"&&preg_match("~Query|Killed~",$K["Command"]))||(DIALECT=="pgsql"&&$t=="query")||(DIALECT=="oracle"&&$t=="sql_text"))?"<code class='jush-".DIALECT."'>".truncate_utf8($X,100).'</code> <a href="'.h(ME.($K["db"]!=""?"db=".urlencode($K["db"])."&":"")."sql=".urlencode($X)).'">'.icon("edit").lang(280).'</a>':h($X));echo"\n";}if($p>=0)echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});");echo"</table>\n","</div>\n","<p>";if(support("kill"))echo($p+1)."/".lang(281,max_connections()),"<p><input type='submit' class='button' value='".lang(282)."'>\n";echo
input_token(),"</p>\n","</form>\n",script("tableCheck();");}elseif(isset($_GET["select"])){$a=$_GET["select"];$R=table_status1($a);$s=indexes($a);$k=fields($a);$he=column_foreign_keys($a);$Dh=$R["Oid"];$Kj=[];$c=[];$ck=[];$ai=[];$Fl=null;foreach($k
as$t=>$j){$A=Admin::get()->getFieldName($j);$lh=html_entity_decode(strip_tags($A),ENT_QUOTES);if(isset($j["privileges"]["select"])&&$A!=""){$c[$t]=$lh;if(is_shortable($j))$Fl=Admin::get()->processSelectionLength();}if(isset($j["privileges"]["where"])&&$A!="")$ck[$t]=$lh;if(isset($j["privileges"]["order"])&&$A!="")$ai[$t]=$lh;$Kj+=$j["privileges"];}list($M,$_e)=Admin::get()->processSelectionColumns($c,$s);$M=array_unique($M);$_e=array_unique($_e);$Bf=count($_e)<count($M);$Z=Admin::get()->processSelectionSearch($k,$s);$D=Admin::get()->processSelectionOrder($k,$s);$v=Admin::get()->processSelectionLimit();if($_GET["modify"]&&!Admin::get()->isDataEditAllowed())redirect(ME."select=".urlencode($a));if($_GET["val"]&&is_ajax()){header("Content-Type: text/plain; charset=utf-8");foreach($_GET["val"]as$km=>$K){$La=convert_field($k[key($K)]);$M=[$La?:idf_escape(key($K))];$Z[]=where_check($km,$k);$J=Driver::get()->select($a,$M,$Z,$M);if($J)echo
first($J->fetchRow());}exit;}$aj=$nm=[];foreach($s
as$r){if($r["type"]=="PRIMARY"){$aj=array_flip($r["columns"]);$nm=($M?$aj:[]);foreach($nm
as$t=>$X){if(in_array(idf_escape($t),$M))unset($nm[$t]);}break;}}if($Dh&&!$aj){$aj=$nm=[$Dh=>0];$s[]=["type"=>"PRIMARY","columns"=>[$Dh]];}$O=Admin::get()->getSettings();if($_POST){$Um=$Z;if(!$_POST["all"]&&is_array($_POST["check"])){$ub=[];foreach($_POST["check"]as$qb)$ub[]=where_check($qb,$k);$Um[]="((".implode(") OR (",$ub)."))";}$Um=($Um?"\nWHERE ".implode(" AND ",$Um):"");if($_POST["export"]){$O->updateParameters(["exportFormat"=>$_POST["format"],"exportOutput"=>$_POST["output"],]);dump_headers($a);Admin::get()->dumpTable($a,"");$pe=($M?implode(", ",$M):"*").convert_fields($c,$k,$M)."\nFROM ".table($a);$Ce=($_e&&$Bf?"\nGROUP BY ".implode(", ",$_e):"").($D?"\nORDER BY ".implode(", ",$D):"");if(!is_array($_POST["check"])||$aj)$H="SELECT $pe$Um$Ce";else{$hm=[];foreach($_POST["check"]as$X)$hm[]="(SELECT".limit($pe,"\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$k).$Ce,1).")";$H=implode(" UNION ALL ",$hm);}Admin::get()->dumpData($a,"table",$H);exit;}if($_POST["save"]||$_POST["delete"]){$I=true;$za=0;$wk=[];if(!$_POST["delete"]){$kk=array_keys($_POST["fields"]+$_POST["function"]);foreach($kk
as$A){$X=process_input($k[$A]);if($X!==null&&($_POST["clone"]||$X!==false))$wk[idf_escape($A)]=($X!==false?$X:idf_escape($A));}}if($_POST["delete"]||$wk){if($_POST["clone"])$H="INTO ".table($a)." (".implode(", ",array_keys($wk)).")\nSELECT ".implode(", ",$wk)."\nFROM ".table($a);if($_POST["all"]||($aj&&is_array($_POST["check"]))||$Bf){$I=($_POST["delete"]?Driver::get()->delete($a,$Um):($_POST["clone"]?queries("INSERT $H$Um".Driver::get()->getInsertReturningSql($a)):Driver::get()->update($a,$wk,$Um)));$za=Connection::get()->getAffectedRows();if(is_object($I))$za+=$I->getRowsCount();}else{foreach((array)$_POST["check"]as$X){$Tm="\nWHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($X,$k);$I=($_POST["delete"]?Driver::get()->delete($a,$Tm,1):($_POST["clone"]?queries("INSERT".limit1($a,$H,$Tm)):Driver::get()->update($a,$wk,$Tm,1)));if(!$I)break;$za+=Connection::get()->getAffectedRows();}}}$_=lang(283,$za);if($_POST["clone"]&&$I&&$za==1){$cg=last_id($I);if($cg)$_=lang(212," $cg");}queries_redirect(remove_from_uri($_POST["all"]&&$_POST["delete"]?"page":""),$_,(bool)$I);if(!$_POST["delete"]){$hd=array_filter($k,function($j){return!(isset($j["generated"])?$j["generated"]:null);});edit_form($a,$hd,(array)$_POST["fields"],!$_POST["clone"]);page_footer();exit;}}elseif(!$_POST["import"]){if(!$_POST["val"])Admin::get()->addError(lang(284));else{$Yk=true;$za=0;foreach($_POST["val"]as$km=>$K){$wk=[];foreach($K
as$t=>$X){$t=bracket_escape($t,true);$wk[idf_escape($t)]=(preg_match('~char|text~',$k[$t]["type"])||$X!=""?Admin::get()->processFieldInput($k[$t],$X):"NULL");}$Yk=(bool)Driver::get()->update($a,$wk," WHERE ".($Z?implode(" AND ",$Z)." AND ":"").where_check($km,$k),($Bf||$aj?0:1)," ");if(!$Yk)break;$za+=Connection::get()->getAffectedRows();}queries_redirect(remove_from_uri(),lang(283,$za),$Yk);}}elseif(!is_string($l=get_file("csv_file",true)))Admin::get()->addError(upload_error($l));elseif(!preg_match('~~u',$l))Admin::get()->addError(lang(285));else{$O->updateParameter("exportFormat",$_POST["import_format"]);$Gb=array_keys($k);preg_match_all('~(?>"[^"]*"|[^"\r\n]+)+~',$l,$z);$za=count($z[0]);Driver::get()->begin();$lk=($_POST["import_format"]=="csv;"?";":($_POST["import_format"]=="tsv"?"\t":","));$L=[];foreach($z[0]as$t=>$X){preg_match_all("~((?>\"[^\"]*\")+|[^$lk]*)$lk~",$X.$lk,$Bg);if(!$t&&!array_diff($Bg[1],$Gb)){$Gb=$Bg[1];$za--;}else{$wk=[];foreach($Bg[1]as$p=>$_b)$wk[idf_escape($Gb[$p])]=($_b==""&&$k[$Gb[$p]]["null"]?"NULL":q(preg_match('~^".*"$~s',$_b)?str_replace('""','"',substr($_b,1,-1)):$_b));$L[]=$wk;}}$Yk=!$L||Driver::get()->insertUpdate($a,$L,$aj);if($Yk)Driver::get()->commit();queries_redirect(remove_from_uri("page"),lang(286,$za),$Yk);Driver::get()->rollback();}}$ol=Admin::get()->getTableName($R);if(is_ajax()){page_headers();ob_start();}else
page_header(lang(56).": $ol",[$ol]);$sf=null;if(isset($Kj["insert"])||!support("table")){$sf=[];foreach((array)$_GET["where"]as$X){if(isset($he[$X["col"]])&&count($he[$X["col"]])==1&&($X["op"]=="="||(!$X["op"]&&(is_array($X["val"])||!preg_match('~[_%]~',$X["val"])))))$sf["preset"."[".bracket_escape($X["col"])."]"]=$X["val"];}}Admin::get()->printTableMenu($R,$sf);if(!$c&&support("table"))echo"<p class='error'>".lang(287).($k?".":": ".error())."\n";else{echo"<form id='form' action=''>\n","<div hidden>";hidden_fields_get();if(DB!=""){echo
input_hidden("db",DB);if(isset($_GET["ns"]))echo
input_hidden("ns",$_GET["ns"]);}echo
input_hidden("select",$a),"<input type='submit' class='button' value='".lang(56)."'>","</div>\n","<div class='field-sets'>\n";Admin::get()->printSelectionColumns($M,$c);Admin::get()->printSelectionSearch($Z,$ck,$s);Admin::get()->printSelectionOrder($D,$ai,$s);Admin::get()->printSelectionLimit($v);Admin::get()->printSelectionLength($Fl);Admin::get()->printSelectionAction($s);echo"</div>\n</form>\n";$E=isset($_GET["page"])?$_GET["page"]:null;if($E=="last"){$ne=Connection::get()->getValue(count_rows($a,$Z,$Bf,$_e));$E=(int)floor(max(0,intval($ne)-1)/$v);}else{$ne=false;$E=(int)$E;}$dk=$M;$Ae=$_e;if(!$dk){$dk[]="*";$bc=convert_fields($c,$k,$M);if($bc)$dk[]=substr($bc,2);}foreach($M
as$t=>$X){$j=$k[idf_unescape($X)];if($j&&($La=convert_field($j)))$dk[$t]="$La AS $X";}if(DIALECT=="pgsql"||DIALECT=="mssql"){foreach((array)$_GET["columns"]as$t=>$X){if(isset($dk[$t])&&$X["fun"])$dk[$t].=" AS ".idf_escape(apply_sql_function($X["fun"],($X["col"]!=""?$X["col"]:"*")));}}if(!$Bf&&$nm){foreach($nm
as$t=>$X){$dk[]=idf_escape($t);if($Ae)$Ae[]=idf_escape($t);}}$I=Driver::get()->select($a,$dk,$Z,$Ae,$D,$v,$E,true);if(!$I)echo"<p class='error'>".error()."\n";else{if(DIALECT=="mssql"&&$E)$I->seek($v*$E);$L=[];while($K=$I->fetchAssoc()){if($E&&DIALECT=="oracle")unset($K["RNUM"]);$L[]=$K;}if($_GET["modify"]&&$L){$Jg=max_input_vars(count($L[0])+1,20);echo($Jg&&count($L)>$Jg?"<p class='error'>".max_input_vars_error()."\n":"");}echo"<form id='selection_form' action='' method='post' enctype='multipart/form-data'>\n","<div class='table-footer-parent'>\n";if($_GET["page"]!="last"&&$v&&$_e&&$Bf&&DIALECT=="sql")$ne=Connection::get()->getValue(" SELECT FOUND_ROWS()");$id=false;if(!$L)echo"<p class='message'>".lang(90)."\n";else{$Va=Admin::get()->getBackwardKeys($a,$ol);echo"<div class='scrollable'>\n","<table id='table' class='nowrap checkable'>\n","<thead><tr>";if($_e||!$M){echo"<th class='actions'><input type='checkbox' id='all-page' class='jsonly' title='".lang(288)."'>".script("gid('all-page').onclick = partial(formCheck, /^check/);","");if(Admin::get()->isDataEditAllowed())echo" <a href='",h($_GET["modify"]?remove_from_uri("modify"):$_SERVER["REQUEST_URI"]."&modify=1")."' title='",lang(289),"'>",icon_solo("edit-all"),"</a>";}$nh=[];$se=[];reset($M);$qj=1;foreach($L[0]as$t=>$X){if(!isset($nm[$t])){$fk=key($M);$X=isset($_GET["columns"][$fk])?$_GET["columns"][$fk]:[];$j=$k[$M?($X?$X["col"]:current($M)):$t];$A=($j?Admin::get()->getFieldName($j,$qj):(isset($X["fun"])?"*":h($t)));if($A!=""){$qj++;$nh[$t]=$A;$b=idf_escape($t);$We=remove_from_uri('(order|desc)[^=]*|page').'&order%5B0%5D='.urlencode($t);$Cc="&desc%5B0%5D=1";$Zh=isset($D[0])?$D[0]:"";$Ek=preg_replace('~ DESC( NULLS LAST)?$~','',$Zh);$Gk=($Ek==$b||$Ek==$t);echo"<th id='th[".h(bracket_escape($t))."]'".($Gk?" aria-sort='".($Ek==$Zh?"ascending":"descending")."'":"").">";$re=apply_sql_function(isset($X["fun"])?$X["fun"]:null,$A);$Fk=isset($j["privileges"]["order"])||(isset($X["fun"])?$X["fun"]:null);if($Fk)echo'<a href="',h($We.($Gk&&$Ek==$Zh?$Cc:'')),'">',"$re</a>";else
echo$re;echo"<span class='column'>";if($Fk)echo"<a href='".h($We.$Cc)."' title='".lang(63)."' class='button light'>",icon_solo("arrow-down"),"</a>";if(!isset($X["fun"])&&isset($j["privileges"]["where"]))echo"<a href='#fieldset-search' title='".lang(60)."' class='button light jsonly'>",icon_solo("search"),"</a>",script("qsl('a').onclick = partial(selectSearch, '".js_escape($t)."');");echo"</span>";}$se[$t]=isset($X["fun"])?$X["fun"]:null;next($M);}}$kg=[];if($_GET["modify"]){foreach($L
as$K){foreach($K
as$t=>$X)$kg[$t]=max($kg[$t],min(40,strlen(utf8_decode($X))));}}if($Va)echo"<th>".lang(19)."</th>";echo"</thead>\n","<tbody>\n";if(is_ajax())ob_end_clean();foreach(Admin::get()->fillForeignDescriptions($L,$he)as$jh=>$K){$jm=unique_array($L[$jh],$s);if(!$jm){$jm=[];reset($M);foreach($L[$jh]as$t=>$X){if(!preg_match('~^(COUNT|AVG|GROUP_CONCAT|MAX|MIN|SUM)\(~',current($M)))$jm[$t]=$X;next($M);}}$km="";foreach($jm
as$t=>$X){$j=isset($k[$t])?$k[$t]:null;$Af=$j&&is_blob($j);if((DIALECT=="sql"||DIALECT=="pgsql")&&$j&&($Af||preg_match('~char|text|enum|set~',$j["type"]))&&strlen($X)>64){$t=(strpos($t,'(')?$t:idf_escape($t));$t="MD5(".($Af||DIALECT!='sql'||preg_match("~^utf8~",isset($j["collation"])?$j["collation"]:"")?$t:"CONVERT($t USING ".charset(Connection::get()).")").")";$X=md5($Af?(string)Connection::get()->formatValue($X,$j):$X);}$km
.="&".($X!==null?urlencode("where[".bracket_escape($t)."]")."=".urlencode($X===false?"f":$X):"null%5B%5D=".urlencode($t));}echo"<tr>";if($_e||!$M){echo"<td class='actions'>",checkbox("check[]",substr($km,1),in_array(substr($km,1),(array)$_POST["check"]));if(!$Bf&&Admin::get()->isDataEditAllowed())echo" <a href='",h(ME."edit=".urlencode($a).$km),"' class='edit' title='",lang(39),"'>",icon_solo("edit"),"</a>";}reset($M);foreach($K
as$t=>$X){if(isset($nh[$t])){$b=current($M);$j=isset($k[$t])?$k[$t]:null;$w="";if($j&&is_blob($j)&&$X!="")$w=ME.'download='.urlencode($a).'&field='.urlencode($t).$km;if(!$w&&$X!==null){foreach((array)$he[$t]as$n){if(count($he[$t])==1||end($n["source"])==$t){$w="";foreach($n["source"]as$p=>$Hk)$w
.=where_link($p,$n["target"][$p],$L[$jh][$Hk]);$w=($n["db"]!=""?preg_replace('~([?&]db=)[^&]+~','\1'.urlencode($n["db"]),ME):ME).'select='.urlencode($n["table"]).$w;if($n["ns"])$w=preg_replace('~([?&]ns=)[^&]+~','\1'.urlencode($n["ns"]),$w);if(count($n["source"])==1)break;}}}if($b=="COUNT(*)"){$w=ME."select=".urlencode($a);$p=0;foreach((array)$_GET["where"]as$W){if(!array_key_exists($W["col"],$jm))$w
.=where_link($p++,$W["col"],$W["val"],$W["op"]);}foreach($jm
as$Lf=>$W)$w
.=where_link($p++,$Lf,$W);}$zh=$X===null;$Xe=select_value($X,$w,$j,$Fl);$vd=bracket_escape($t);$q=h("val[$km][$vd]");$Vi=isset($_POST["val"][$km][$vd])?$_POST["val"][$km][$vd]:null;$pm=isset($j["privileges"]["update"])?$j["privileges"]["update"]:false;$gd=!is_array($X)&&!($j&&is_blob($j))&&is_utf8((string)$X)&&$L[$jh][$t]==$X&&!$se[$t]&&!(isset($j["generated"])?$j["generated"]:false);$U=($b&&preg_match('~^(AVG|MIN|MAX)\((.+)\)~',$b,$z)?$k[idf_unescape($z[2])]["type"]:(isset($j["type"])?$j["type"]:null));$dh=$U=="money"||($b&&preg_match('~^SUM\((.+)\)~',$b,$z)&&$k[idf_unescape($z[1])]["type"])=="money";$Dl=$U&&preg_match('~text|json|lob~',$U);$Ah=($U&&preg_match(number_type(),$U))||($b&&preg_match('~^(CHAR_LENGTH|ROUND|FLOOR|CEIL|UNIX_TIMESTAMP|TIME_TO_SEC|COUNT|SUM)\(~',$b));$yb=$Ah&&($zh||is_numeric(strip_tags($Xe))||$dh)?"class='number'":"";echo"<td id='$q' $yb";if(($_GET["modify"]&&$gd&&!$zh)||$Vi!==null){$id=true;$Fe=h($Vi!==null?$Vi:$X);echo" data-editing='true'>".($Dl?"<textarea name='$q' cols='30' rows='".(substr_count($X,"\n")+1)."'>$Fe</textarea>":"<input class='input' name='$q' value='$Fe' size='$kg[$t]'>");}else{$zg=strpos($Xe,"<i>…</i>");if($pm)echo" data-text='".($zg?2:($Dl?1:0))."'".($gd?"":" data-warning='".lang(290)."'");echo">$Xe";}}next($M);}if($Va){echo"<td>";Admin::get()->printBackwardKeys($Va,$L[$jh]);echo"</td>";}echo"</tr>\n";}if(is_ajax())exit;echo"</tbody>\n",script("mixin(qs('#table tbody'), {onclick: event => tableClick(event, false, ".(Admin::get()->isDataEditAllowed()?"true":"false")."), ondblclick: event => tableClick(event, true), onkeydown: onEditingKeydown});"),"</table>\n",script("initToggles(gid('table'));"),"</div>\n";}if(!is_ajax()){if($L||$E){$xd=true;if($_GET["page"]!="last"){if(!$v||(count($L)<$v&&($L||!$E)))$ne=($E?$E*$v:0)+count($L);elseif(DIALECT!="sql"||!$Bf){$ne=($Bf?false:found_rows($R,$Z));if($ne<max(1e4,2*($E+1)*$v))$ne=first(slow_query(count_rows($a,$Z,$Bf,$_e)));elseif(DIALECT=='sql'||DIALECT=='pgsql')$xd=false;}}$ri=($v!==null&&($ne===false||$ne>$v||$E));if($ri){if(($ne===false?count($L)+1:$ne-$E*$v)>$v)echo'<p class="links">','<a href="',h(remove_from_uri("page")."&page=".($E+1)),'" class="loadmore">',icon("expand"),lang(291),'</a>',script("qsl('a').onclick = partial(loadNextPage, $v, '".js_escape(lang(292))."');","");echo"\n";}echo"<div class='table-footer'><div class='field-sets'>\n";if($ri){$Hg=($ne===false?$E+(count($L)>=$v?2:1):(int)floor(($ne-1)/$v));$Sc="<li>…</li>";echo"<fieldset><legend>".lang(293)."</legend>";if(DIALECT!="simpledb"){echo"<div id='fieldset-pagination' class='fieldset-content'><ul class='pagination'>",pagination(0,$E);if($E>5)echo$Sc;for($p=max(1,$E-4);$p<min($Hg,$E+5);$p++)echo
pagination($p,$E);if($Hg>0){if($E+5<$Hg)echo$Sc;echo($xd&&$ne!==false?pagination($Hg,$E):" <a href='".h(remove_from_uri("page")."&page=last")."' title='~$Hg'>".lang(294)."</a>");}echo"</ul></div>";}else{echo"<div id='fieldset-pagination'><ul class='pagination'>",pagination(0,$E);if($E>1)echo$Sc;if($E)echo
pagination($E,$E);if($Hg>$E){echo
pagination($E+1,$E);if($Hg>$E+1)echo$Sc;}echo"</ul></div>";}echo"</fieldset>\n";}echo"<fieldset>","<legend>".lang(295)."</legend><div class='fieldset-content'>";$Lc=($xd?"":"~ ").$ne;echo
checkbox("all",1,0,($ne!==false?($xd?"":"~ ").lang(194,$ne):""),"countRows.call(this, '$Lc');")."\n","</div></fieldset>\n";if(Admin::get()->isDataEditAllowed()){echo"<fieldset",($_GET["modify"]?'':' class="jsonly"'),">","<legend>",lang(289),"</legend>";$Sj=($_GET["modify"]?"":" data-inline-edit='1'".($id?"":" disabled"));echo"<div class='fieldset-content'",($_GET["modify"]?"":" title='".lang(284)."'"),">","<input type='submit' class='button' id='modify-save' value='",lang(115),"'",$Sj,">","</div>","</fieldset>\n","<fieldset>","<legend>",lang(165)," <span id='selected'></span></legend>","<div class='fieldset-content'>","<input type='submit' class='button' name='edit' value='",lang(39),"'> ","<input type='submit' class='button' name='clone' value='",lang(280),"'> ","<input type='submit' class='button' name='delete' value='",lang(119),"'>",confirm(),"</div>","</fieldset>\n";}$je=Admin::get()->getDumpFormats();foreach((array)$_GET["columns"]as$b){if($b["fun"]){unset($je['sql']);break;}}if($je){print_fieldset_start("export",lang(75)." <span id='selected2'></span>","export");echo
html_select("format",$je,$O->getParameter("exportFormat"));$oi=Admin::get()->getDumpOutputs();echo($oi?" ".html_select("output",$oi,$O->getParameter("exportOutput")):"")," <input type='submit' class='button' name='export' value='".lang(75)."'>\n";print_fieldset_end("export");}echo"</div></div>\n",script("initTableFooter()");}echo"</div>\n";if(Admin::get()->isDataEditAllowed()){echo"<p>","<a href='#import'>",icon("import"),lang(74),"</a>",script("qsl('a').onclick = partial(toggle, 'import');",""),"</p>","<p id='import'",($_POST["import"]?"":" class='hidden'"),">";if(ini_bool("file_uploads"))echo"<input type='file' name='csv_file'> ",html_select("import_format",["csv"=>"CSV,","csv;"=>"CSV;","tsv"=>"TSV"],$O->getParameter("exportFormat"))," <input type='submit' class='button default' name='import' value='".lang(74)."'>",file_upload_form_script("selection_form","csv_file");else
echo
lang(201);echo"</p>";}echo
input_token(),"</form>\n",(!$_e&&$M?"":script("tableCheck();"));}else
echo"</div>\n";}}if(is_ajax()){ob_end_clean();exit;}}elseif(isset($_GET["variables"])){$P=isset($_GET["status"]);$T=$P?lang(158):lang(157);page_header($T,[$T]);$Dm=($P?Admin::get()->getStatusVariables():Admin::get()->getServerVariables());if(!$Dm)echo"<p class='message'>",lang(90),"</p>\n";else{echo"<div class='scrollable'><table>\n";foreach($Dm
as$K){echo"<tr>";$t=array_shift($K);echo"<th><code class='jush-".DIALECT.($P?"status":"set")."'>".h($t)."</code></th>";foreach($K
as$X)echo"<td>",nl2br(h($X)),"</td>";echo"</tr>\n";}echo"</table></div>\n";}}elseif(isset($_GET["script"])){header("Content-Type: text/javascript; charset=utf-8");if($_GET["script"]=="db"){$bl=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$e=[];$rc=null;foreach(table_status()as$A=>$R){$e["Comment-$A"]=h($R["Comment"]);if(!is_view($R)||preg_match('~materialized~i',$R["Engine"])){$e["Engine-$A"]=h($R["Engine"]);$Bb=isset($R["Collation"])?$R["Collation"]:"";if($Bb==""){if($rc===null)$rc=db_collation(DB,collations())??"";$Bb=$rc;}$e["Collation-$A"]=h($Bb);foreach($bl+["Auto_increment"=>0,"Rows"=>0]as$t=>$X){if($R[$t]!=""){$X=format_number($R[$t]);if($X>=0)$e["$t-$A"]=($t=="Rows"?format_rows($R):$X);if(isset($bl[$t]))$bl[$t]+=($R["Engine"]!="InnoDB"||$t!="Data_free"?$R[$t]:0);}elseif(array_key_exists($t,$R))$e["$t-$A"]="?";}}}if(function_exists('AdminNeo\db_status'))$bl=db_status();foreach($bl
as$t=>$X)$e["sum-$t"]=format_number($X);echo
json_encode($e,JSON_UNESCAPED_UNICODE);}elseif($_GET["script"]=="kill")Connection::get()->query("KILL ".number($_POST["kill"]));else{$e=[];foreach(count_tables(Admin::get()->getDatabases(false))as$g=>$X){$e["tables-$g"]=$X;$e["size-$g"]=db_size($g);}echo
json_encode($e,JSON_UNESCAPED_UNICODE);}exit;}else{$xl=array_merge((array)$_POST["tables"],(array)$_POST["views"]);if($xl&&!$_POST["search"]){$I=true;$_="";if(DIALECT=="sql"&&$_POST["tables"]&&count($_POST["tables"])>1&&($_POST["drop"]||$_POST["truncate"]||$_POST["copy"]))queries("SET foreign_key_checks = 0");if($_POST["truncate"]){if($_POST["tables"])$I=truncate_tables($_POST["tables"]);$_=lang(296);}elseif($_POST["move"]){$I=move_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$_=lang(297);}elseif($_POST["copy"]){$I=copy_tables((array)$_POST["tables"],(array)$_POST["views"],$_POST["target"]);$_=lang(298);}elseif($_POST["drop"]){if($_POST["views"])$I=drop_views($_POST["views"]);if($I&&$_POST["tables"])$I=drop_tables($_POST["tables"]);$_=lang(299);}elseif(DIALECT=="sqlite"&&$_POST["check"]){foreach((array)$_POST["tables"]as$Q){foreach(get_rows("PRAGMA integrity_check(".q($Q).")")as$K)$_
.="<b>".h($Q)."</b>: ".h($K["integrity_check"])."<br>";}}elseif(DIALECT!="sql"){$I=(DIALECT=="sqlite"?queries("VACUUM"):apply_queries("VACUUM".($_POST["optimize"]?" ANALYZE":""),(array)$_POST["tables"]));$_=lang(300);}elseif(!$_POST["tables"])$_=lang(79);elseif($I=queries(($_POST["optimize"]?"OPTIMIZE":($_POST["check"]?"CHECK":($_POST["repair"]?"REPAIR":"ANALYZE")))." TABLE ".implode(", ",array_map('AdminNeo\idf_escape',$_POST["tables"])))){while($K=$I->fetchAssoc())$_
.="<b>".h($K["Table"])."</b>: ".h($K["Msg_text"])."<br>";}queries_redirect($_SERVER["REQUEST_URI"],$_,(bool)$I);}if($_GET["ns"]=="")page_header(lang(31).": ".h(DB),true);else
page_header(lang(189).": ".h($_GET["ns"]),true);Admin::get()->printDatabaseMenu();if($_GET["ns"]===""){echo"<h2 id='schemas'>".lang(301)."</h2>\n";$Zj=Admin::get()->getSchemas();if(!$Zj)echo"<p class='message'>".lang(302)."\n";else{echo"<div class='scrollable'>\n","<table class='nowrap'>\n",'<thead><tr class="wrap"><th>',lang(189),"</th></tr></thead>";foreach($Zj
as$A)echo"<tr><th><a href='",h(ME),"ns=".urlencode($A),"' title='",lang(303),"'>".h($A)."</a></th></tr>";echo'</table></div>';}echo'<p class="links"><a href="'.h(ME).'scheme=">'.icon("database-add").lang(77)."</a>\n";}else{echo"<h2 id='tables-views'>".lang(304)."</h2>\n";$sl=['sql'=>'show-table-status.html','mariadb'=>'reference/sql-statements/administrative-sql-statements/show/show-table-status'];$rc=db_collation(DB,collations());$c=["Engine"=>["label"=>lang(169),"doc"=>doc_link(['sql'=>'storage-engines.html','mariadb'=>'server-usage/storage-engines']),],];if($rc!="")$c["Collation"]=["label"=>lang(46),"doc"=>doc_link(['sql'=>'charset-charsets.html','mariadb'=>'reference/data-types/string-data-types/character-sets/supported-character-sets-and-collations']),];$c+=["Data_length"=>["label"=>lang(305),"doc"=>doc_link($sl+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT','oracle'=>'REFRN20286']),"link"=>"create","title"=>lang(36),],"Index_length"=>["label"=>lang(306),"doc"=>doc_link($sl+['pgsql'=>'functions-admin.html#FUNCTIONS-ADMIN-DBOBJECT']),"link"=>"indexes","title"=>lang(173),],"Data_free"=>["label"=>lang(307),"doc"=>doc_link($sl),"link"=>"edit","title"=>lang(8),],"Auto_increment"=>["label"=>lang(48),"doc"=>doc_link(['sql'=>'example-auto-increment.html','mariadb'=>'reference/data-types/auto_increment']),"link"=>"auto_increment=1&create","title"=>lang(36),],"Rows"=>["label"=>lang(308),"doc"=>doc_link($sl+['pgsql'=>'catalog-pg-class.html#CATALOG-PG-CLASS','oracle'=>'REFRN20286']),"link"=>"select","title"=>lang(34),],];if(support("comment"))$c["Comment"]=["label"=>lang(47),"doc"=>doc_link($sl+['pgsql'=>'functions-info.html#FUNCTIONS-INFO-COMMENT-TABLE']),];$D=(is_string($_GET["order"])?$_GET["order"]:"");$Dc=null;if(preg_match('~^(.+)-(asc|desc)$~',$D,$y)){$D=$y[1];$Dc=($y[2]=="desc");}if($D!="__table"&&!isset($c[$D]))$D="";if($Dc===null)$Dc=isset($c[$D]["link"]);$Xm=($D!=""&&$D!="__table")||support("fast_status");$vl=($Xm?table_status():tables_list());if(!$vl)echo"<p class='message'>".lang(79)."\n";else{echo"<form action='' method='post'>\n","<div class='table-footer-parent'>\n";if(support("table")){echo"<div class='field-sets'>\n","<fieldset><legend>".lang(309)." <span id='selected2'></span></legend><div class='fieldset-content'>",html_select("op",Admin::get()->getOperators(),isset($_POST["op"])?$_POST["op"]:Driver::get()->getLikeOperator()),"<input type='search' class='input' name='query' value='".h($_POST["query"])."'>",script("qsl('input').onkeydown = event => bodyKeydown(event, 'search');","")," <input type='submit' class='button' name='search' value='".lang(60)."'>\n","</div></fieldset>\n","</div>\n";if($_POST["search"]&&$_POST["query"]!=""){$_GET["where"][0]["op"]=$_POST["op"];search_tables();}}echo"<div class='scrollable'>\n","<table class='nowrap checkable'>\n",'<thead><tr class="wrap">','<th class="actions"><input id="check-all" type="checkbox" class="input jsonly" title="'.lang(188).'">'.script("gid('check-all').onclick = partial(formCheck, /^(tables|views)\[/);","");$kh=($D==""||$D=="__table");$nl=($kh&&!$Dc?ME."order=__table-desc":substr(ME,0,-1));$mh=($kh&&DIALECT!="sqlite");echo'<th'.($mh?" aria-sort='".($Dc?"descending":"ascending")."'":'').'><a href="'.h($nl).'">'.lang(9).'</a>';foreach($c
as$t=>$b){$Fc=($t===$D?!$Dc:isset($b["link"]));echo'<th'.($t===$D?" aria-sort='".($Dc?"descending":"ascending")."'":'').'><a href="'.h(ME)."order=$t-".($Fc?"desc":"asc").'">'.$b["label"].'</a>'.$b["doc"];}echo"</thead>\n","<tbody>\n";if($D=="__table"){if($Dc)$vl=array_reverse($vl,true);}elseif($D){uasort($vl,function($sa,$Sa)use($D,$Dc){$Ym=isset($sa[$D])?$sa[$D]:null;$Zm=isset($Sa[$D])?$Sa[$D]:null;$I=($Ym<$Zm?-1:($Ym>$Zm?1:0));return($Dc?-$I:$I);});}$bl=["Data_length"=>0,"Index_length"=>0,"Data_free"=>0];$S=0;foreach($vl
as$A=>$P){$Hm=($Xm?is_view($P):$P!==null&&!preg_match('~table|sequence~i',$P));$od=($Xm?(isset($P["Engine"])?$P["Engine"]:""):$P);$q=h("Table-".$A);echo'<tr><th class="actions">'.checkbox(($Hm?"views[]":"tables[]"),$A,in_array("$A",$xl,true),"","","",$q);if(!Admin::get()->getSettings()->isSelectionPreferred()&&(support("table")||support("indexes")))$ua="table";else$ua="select";echo"<th><a href='",h(ME),"$ua=",urlencode($A),"' id='$q'>",h($A),"</a></th>";if($Hm&&!preg_match('~materialized~i',$od)){$T=lang(168);$Hb=count($c)-(support("comment")?2:1);echo'<td colspan="'.$Hb.'">'.(support("view")?"<a href='".h(ME)."view=".urlencode($A)."' title='".lang(37)."'>$T</a>":$T),"<td align='right'><a href='".h(ME)."select=".urlencode($A)."' title='".lang(34)."'>?</a>";}else{foreach($c
as$t=>$b){if($t=="Comment")continue;$q=" id='$t-".h($A)."'";$w=isset($b["link"])?$b["link"]:"";if(!$w){$X="";if($Xm){$X=isset($P[$t])?$P[$t]:"";if($t=="Collation"&&$X=="")$X=$rc;}echo"<td$q>".h($X);continue;}$X="?";if($Xm){$B=isset($P[$t])?$P[$t]:"";if(is_numeric($B)&&$B>=0){$X=($t=="Rows"?format_rows($P):format_number($B));if(isset($bl[$t])&&($od!="InnoDB"||$t!="Data_free"))$bl[$t]+=$B;}}echo"<td align='right'>".(support("table")||$t=="Rows"||(support("indexes")&&$t!="Data_length")?"<a href='".h(ME."$w=").urlencode($A)."'$q title='".$b["title"]."'>".h($X)."</a>":"<span$q>".h($X)."</span>");}$S++;}echo(support("comment")?"<td id='Comment-".h($A)."'>".($Xm?h(isset($P["Comment"])?$P["Comment"]:""):""):""),"\n";}echo"</tbody>\n",script("mixin(qsl('tbody'), {onclick: tableClick, ondblclick: event => tableClick(event, true)});"),"<tfoot><tr>","<td><th>".lang(281,count($vl)),"<td>".h(DIALECT=="sql"?Connection::get()->getValue("SELECT @@default_storage_engine"):""),($rc!=""?"<td>".h($rc):"");if($Xm&&function_exists('AdminNeo\db_status'))$bl=db_status();foreach($bl
as$t=>$al)echo"<td align='right' id='sum-$t'>".($Xm?format_number($al):"");echo"<td></td><td></td>";if(support("comment"))echo"<td></td>";echo"</tr></tfoot>\n","</table>\n","</div>\n",($Xm?"":script("ajaxSetHtml('".js_escape(ME)."script=db');"));if(Admin::get()->isDataEditAllowed()){echo"<div class='table-footer'><div class='field-sets'>\n";$Am="<input type='submit' class='button' value='".lang(310)."'> ".help_script("VACUUM");$Vh="<input type='submit' class='button' name='optimize' value='".lang(311)."'> ".help_script(DIALECT=="sql"?"OPTIMIZE TABLE":"VACUUM ANALYZE");echo"<fieldset><legend>".lang(165)." <span id='selected'></span></legend><div class='fieldset-content'>".(DIALECT=="sqlite"?$Am."<input type='submit' class='button' name='check' value='".lang(312)."'> ".help_script("PRAGMA integrity_check"):(DIALECT=="pgsql"?$Am.$Vh:(DIALECT=="sql"?"<input type='submit' class='button' value='".lang(313)."'> ".help_script("ANALYZE TABLE").$Vh."<input type='submit' class='button' name='check' value='".lang(312)."'> ".help_script("CHECK TABLE")."<input type='submit' class='button' name='repair' value='".lang(314)."'> ".help_script("REPAIR TABLE"):"")))."<input type='submit' class='button' name='truncate' value='".lang(315)."'> ".help_script(DIALECT=="sqlite"?"DELETE":("TRUNCATE".(DIALECT=="pgsql"?"":" TABLE"))).confirm()."<input type='submit' class='button' name='drop' value='".lang(166)."'>".help_script("DROP TABLE").confirm()."\n";$f=(support("scheme")?Admin::get()->getSchemas():Admin::get()->getDatabases());echo"</div></fieldset>\n";if(count($f)!=1&&DIALECT!="sqlite"){echo"<fieldset><legend>".lang(316)." <span id='selected3'></span></legend><div>";$g=(isset($_POST["target"])?$_POST["target"]:(support("scheme")?$_GET["ns"]:DB));echo($f?html_select("target",$f,$g,"","label-move"):'<input class="input" name="target" value="'.h($g).'" autocapitalize="off">')," <input type='submit' class='button' name='move' value='".lang(317)."'>",(support("copy")?" <input type='submit' class='button' name='copy' value='".lang(318)."'> ".checkbox("overwrite",1,$_POST["overwrite"],lang(319)):""),"</div></fieldset>\n";}echo
input_hidden("all"),script("qsl('input').onclick = partial(countTables, $S);"),input_token(),"</div></div>\n",script("initTableFooter()");}echo"</div>\n","</form>\n",script("tableCheck();");}echo'<p class="links"><a href="',h(ME),'create=">',icon("table-add"),lang(78),"</a>\n";if(support("view"))echo'<a href="',h(ME),'view=">',icon("view-add"),lang(244),"</a>\n";if(support("routine")){echo"<h2 id='routines'>".lang(184)."</h2>\n";$Oj=routines();if($Oj){$Nb=$Oj[0]["ROUTINE_COMMENT"]!==null;echo"<table>\n",'<thead><tr>','<th>',lang(224),'</th><td>',lang(45),'</td><td>',lang(261),"</td>";if($Nb)echo"<td>",lang(47),"</td>";echo"<td></td>","</tr></thead>\n";foreach($Oj
as$K){$A=($K["SPECIFIC_NAME"]==$K["ROUTINE_NAME"]?"":"&name=".urlencode($K["ROUTINE_NAME"]));echo'<tr>','<th><a href="',h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'callf=':'call=').urlencode($K["SPECIFIC_NAME"]).$A),'">',h($K["ROUTINE_NAME"]),'</a></th>','<td>',h($K["ROUTINE_TYPE"]),'</td>','<td>',h($K["DTD_IDENTIFIER"]),'</td>';if($Nb)echo'<td>',truncate_utf8(preg_replace('~\s{2,}~'," ",trim($K["ROUTINE_COMMENT"])),50),'</td>';echo'<td><a href="'.h(ME.($K["ROUTINE_TYPE"]!="PROCEDURE"?'function=':'procedure=').urlencode($K["SPECIFIC_NAME"]).$A).'">'.lang(176)."</a></td>";}echo"</table>\n";}echo'<p class="links">';if(support("procedure"))echo'<a href="',h(ME),'procedure=">',icon("function-add"),lang(260),"</a>";echo'<a href="',h(ME),'function=">',icon("function-add"),lang(259),"</a>\n","</p>\n";}if(support("event")){echo"<h2 id='events'>".lang(185)."</h2>\n";$L=get_rows("SHOW EVENTS");if($L){echo"<table>\n","<thead><tr><th>".lang(224)."<td>".lang(320)."<td>".lang(250)."<td>".lang(251)."<td></thead>\n";foreach($L
as$K)echo"<tr>","<th>".h($K["Name"]),"<td>".($K["Execute at"]?lang(321)."<td>".h($K["Execute at"]):lang(252)." ".h($K["Interval value"])." ".h($K["Interval field"])."<td>".h($K["Starts"])),"<td>".h($K["Ends"]),'<td><a href="'.h(ME).'event='.urlencode($K["Name"]).'">'.lang(176).'</a>';echo"</table>\n";$wd=Connection::get()->getValue("SELECT @@event_scheduler");if($wd&&$wd!="ON")echo"<p class='error'><code class='jush-sqlset'>event_scheduler</code>: ".h($wd)."\n";}echo'<p class="links"><a href="',h(ME),'event=">',icon("event-add"),lang(249),"</a></p>\n";}}}page_footer();