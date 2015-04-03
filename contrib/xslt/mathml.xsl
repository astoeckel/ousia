<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
		version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:math="math">

	<xsl:template match="/">
		<xsl:apply-templates select="/document/math:math"/>
	</xsl:template>

	<xsl:template match="/document/math:math">
		<xsl:element name="math" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:apply-templates select="math:*"/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:equation">
		<!--<xsl:element name="mtr" namespace="http://www.w3.org/1998/Math/MathML">-->
			<xsl:apply-templates select="math:field"/>
		<!--</xsl:element>-->
	</xsl:template>

	<xsl:template match="math:field">
		<xsl:element name="mrow" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:apply-templates select="math:*"/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:power">
		<xsl:element name="msup" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:apply-templates select="math:*"/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:index">
		<xsl:element name="msub" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:apply-templates select="math:*"/>
		</xsl:element>
	</xsl:template>

	<!-- Operators -->

	<xsl:template match="math:equals">
		<xsl:element name="mo" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:text>=</xsl:text>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:plus">
		<xsl:element name="mo" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:text>+</xsl:text>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:minus">
		<xsl:element name="mo" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:text>-</xsl:text>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:ast">
		<xsl:element name="mo" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:text>*</xsl:text>
		</xsl:element>
	</xsl:template>

	<!-- Leaf elements -->

	<xsl:template match="math:number">
		<xsl:element name="mn" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:value-of select="."/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:text">
		<xsl:element name="mtext" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:value-of select="."/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="math:var">
		<xsl:element name="mi" namespace="http://www.w3.org/1998/Math/MathML">
			<xsl:value-of select="."/>
		</xsl:element>
	</xsl:template>

	<xsl:template math="* msup">
		
	</xsl:template>

</xsl:stylesheet>

