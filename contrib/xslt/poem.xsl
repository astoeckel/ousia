<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
		version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:poem="poem">

	<xsl:output method="html" encoding="utf-8" indent="yes" />

	<xsl:template match="/">
		<html>
			<head>
				<title>Poem</title>
				<style type="text/css">
body {
	font-size: 14pt;
	line-height: 1.75;
	font-family: Palatino, serif;
	color: black;
}
h1, h2, h3 {
	font-size: 100%;
	font-weight: bold;
}
h1 span.title {
	font-size: 150%;
}
h1 span.subtitle {
	display: block;
}
.poem {
	position: relative;
	padding-left: 4em;
}
.poem h2 {
	font-size: 100%;
}
.linenumber {
	position: absolute;
	left: 0;
	width: 2em;
	text-align: right;
	-moz-user-select: none;
	-webkizt-user-select: none;
	user-select: none;
}
.indentation {
	display: block;
	padding-left: 2em;
}
				</style>
			</head>
			<body>
				<xsl:apply-templates select="/document/poem:poem"/>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="/document/poem:poem">
		<section class="poem">
			<h1>
				<span class="title">
					<xsl:value-of select="title"/>
				</span>
				<span class="subtitle">
					<xsl:text>by </xsl:text>
					<a rel="author"><xsl:value-of select="author"/></a>
					<xsl:text> (</xsl:text>
					<span class="year">
						<xsl:value-of select="year"/>
					</span>
					<xsl:text>)</xsl:text>
				</span>
			</h1>
			<xsl:apply-templates select="poem:*"/>
		</section>
	</xsl:template>

	<xsl:template match="poem:stanza">
		<h2><xsl:number level="any" from="poem:poem" format="I."/></h2>
		<p class="stanza">
			<xsl:apply-templates select="poem:*"/>
		</p>
	</xsl:template>

	<xsl:template match="poem:line">
		<xsl:variable name="linenumber"><xsl:number level="any" from="poem:poem"/></xsl:variable>
		<xsl:if test="($linenumber mod 5 = 0) or ($linenumber = 1)">
			<span class="linenumber"><xsl:value-of select="$linenumber"/></span>
		</xsl:if>
		<span class="line">
			<xsl:value-of select="."/>
		</span><br/>
	</xsl:template>

	<xsl:template match="poem:indentation">
		<span class="indentation">
			<xsl:apply-templates select="poem:*"/>
		</span>
	</xsl:template>
</xsl:stylesheet>

