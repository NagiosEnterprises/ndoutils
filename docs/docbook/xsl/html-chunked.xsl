<?xml version="1.0" encoding="euc-kr"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>
	   
<!-- <xsl:import href="http://docbook.sourceforge.net/release/xsl/current/xhtml/chunk.xsl"/> -->

<!-- <xsl:import href="dbk-html-common.xsl"/> -->
<!--<xsl:import href="html/chunk.xsl"/> -->
<xsl:import href="http://docbook.process-zero.de/xsl/html/chunk.xsl"/>

<xsl:output method="xml" encoding="utf-8"/>

<xsl:param name="chunker.output.encoding" select="'utf-8'"/>

<xsl:param name="chunk.first.sections" select="'1'"/>

<!-- <xsl:param name="email.nospam" select="'1'"/> -->

<!-- Hendrik: uses chapter/section ids as filenames -->
<xsl:param name="use.id.as.filename" select="'1'"/>

<!-- Hendrik: uses images for note, tip, warning,caution and important -->
<xsl:param name="admon.graphics.path">../images/</xsl:param>
<xsl:param name="admon.graphics" select="'1'"/>

<!-- Hendrik: use the following relative image path -->
<!-- <xsl:param name="img.src.path">../images/</xsl:param> -->

<!-- Hendrik: we want some nice navigation icons in header and footer -->
<!-- <xsl:param name="navig.graphics" select="'1'"/> -->

<!-- Hendrik: yes - we want nicer html output... not all on one line -->
<xsl:param name="chunker.output.indent" select="'yes'"/>

<!-- Hendrik: don't use image scaling -->
<xsl:param name="ignore.image.scaling" select="'1'"/>

<!-- Hendrik: Here we get our CSS Stylesheet -->
<!--
<xsl:param name="html.stylesheet" select="'nagios.css'" />
-->

<!-- Hendrik: Copyright Information on each page -->

<xsl:template name="user.header.navigation">
<CENTER><IMG src="../images/logofullsize.png" border="0" alt="Nagios" title="Nagios"/></CENTER>
</xsl:template>

<!-- 
<xsl:template name="user.footer.content">
  <HR/><P class="copyright">&#x00A9; 2009 Nagios Development Team, http://www.nagios.org</P>
</xsl:template>
-->
<xsl:template name="user.footer.navigation">
  <P class="copyright">&#x00A9; 2009 Nagios Development Team, http://www.nagios.org</P>
</xsl:template>

</xsl:stylesheet>
