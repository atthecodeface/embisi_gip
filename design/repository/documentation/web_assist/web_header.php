<?php

/*a Include the site map data
 */

include "${toplevel}web_assist/web_site_map.php";

/*a Now output the header with document title
 */
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>

<?php

echo "<title>$page_title</title>\n";

/*a Include the CSS in the header*/
include "${toplevel}web_assist/web_css.php";

?>
</head>

<?php

/*a Now build the structure for the page
  The toplevel structure is a table with five rows, each with 4 columns:
    The top is the logo/general navigation bar
    The second is a horizontal bar
    The third is the links bar and contents of the page
    The fourth is a horizontal bar
    The fifth is the copyright bar and final footer
 */
echo "<body bgcolor=\"$page_background\" leftmargin=0 topmargin=0 marginwidth=0 marginheight=0>\n";

echo "<table border=0 width=100% cellspacing=0 cellpadding=0 bgcolor=\"$body_background\">\n";

/*a First row: logo and general navigation bar
 */
echo "\n<tr height=$logo_height>\n";
echo "<td width=$nav_width bgcolor=\"$logo_background\" valign=top align=left ><img src=\"${toplevel}images/embisi_small.gif\" width=80 height=64 border=0/></td>\n";
echo "<td width=1 bgcolor=\"$logo_background\" valign=top align=center ></td>\n";
echo "<td bgcolor=\"$logo_background\" valign=center align=center class=logo>$page_title</td>\n";
echo "<td width=1 bgcolor=\"$logo_background\" valign=top align=center ></td>\n";
echo "</tr>\n";

/*a Second row: horizontal bar
 */
echo "\n<tr class=nomargin height=1>\n";
echo "<td class=nomargin height=1 bgcolor=\"$nav_background\">\n";
  echo "<table class=nomargin cellspacing=0 width=100%>\n";
    echo "<tr class=nomargin>\n";
      echo "<td width=12 class=nomargin>";
      insert_blank(1,1);
      echo "</td>";
      echo "<td bgcolor=black class=nomargin>";
      insert_blank(1,1);
      echo "</td>\n";
    echo "</tr>\n";
  echo "</table>\n";
echo "</td>\n";
echo "<td colspan=3 bgcolor=black>";
insert_blank(1,1);
echo "</td>\n</tr>\n";

/*a Third  row: links column, divider, main contents, divider
 */
echo "\n<tr>\n";

/*b Navigation column
 */

echo "\n\n<td width=$nav_width bgcolor=\"$nav_background\" valign=top align=center >\n";
include "${toplevel}web_assist/web_navigation.php";
echo "</td>\n";

/*b Vertical bar between the columns
 */
echo "\n<td width=1 height=100% class=nomargin bgcolor=black>";
insert_blank(1,1);
echo "</td>\n";

/*b Main contents - full width, align top, start the main contents table (contents will be set up as rows of a table)
 */
?>

<td valign="top">
<table border=0 width=100%>

