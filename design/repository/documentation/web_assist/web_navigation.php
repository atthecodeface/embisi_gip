<?php

/*a Start a table for the contents to be laid out properly
 */
echo "<table width=100% height=100%>\n";

/*a Insert a break
 */
nav_break();

/*a Insert global navigation area
 */
nav_link_start("");
nav_link_add( "index.php", "Home" );
nav_link_end();

/*a Insert a break
 */
nav_break();

/*a Insert any local navigation if required
 */
function add_sitemap_tag( $posn, $tag, $current_marker )
{
    global $site_map;
    if ($tag=="")
    {
        $max = $site_map["count"][0];
    }
    else
    {
        $max = $site_map["nsublinks"][$tag];
    }
    for ($i=1; $i<=$max; $i++)
    {
        $elt_tag = $tag.".".$i;
        $marker = $current_marker.".".$site_map["markers"][$elt_tag];
        nav_link_add( $site_map["hrefs"][$elt_tag], $site_map["texts"][$elt_tag] );
        if ($site_map["nsublinks"][$elt_tag]>0)
        {
            $mlen = strlen($marker);
            if ( (substr($posn, 0, $mlen)==$marker) &&
                 ( ($posn==$marker) || ($posn[$mlen]==".")) )
            {
                nav_link_push();
                add_sitemap_tag( $posn, $elt_tag, $marker );
                nav_link_pop();
            }
        }
    }
}
if ($site_depth==0)
{
    $site_position = ".top";
}
nav_link_start_hierarchy("Site map");
add_sitemap_tag( $site_position, "", "" );
nav_link_end();

/*a Insert a break
 */
nav_break();

/*a Finish the table
 */
echo "</table>\n";


