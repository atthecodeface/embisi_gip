<?php

/*a End the main column
 */

page_ep();

echo "</table>\n</td>\n";

/*a Final vertical divider
 */
echo "\n<td width=1 height=100% bgcolor=black>";
insert_blank(1,1);
echo "</td>\n";
echo "\n</tr>\n";

/*a Fourth row: dividing black line
 */
echo "\n<tr height=1><td bgcolor=\"$nav_background\"></td><td colspan=3 bgcolor=black>";
insert_blank(1,1);
echo "</td>\n</tr>\n";

/*a Fifth row:
 */
echo "\n<tr><td colspan=4 bgcolor=\"$footer_background\"";

?>
<p class=footer>
This web page copyright &copy; Embisi Inc. 2004
</td>
</tr>

<?php

/*a End table, end page
 */
?>
</table>
</body>
</html>
