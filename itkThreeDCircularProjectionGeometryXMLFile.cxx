#ifndef _itkThreeDCircularProjectionGeometryXMLFile_cxx
#define _itkThreeDCircularProjectionGeometryXMLFile_cxx

#include "itkThreeDCircularProjectionGeometryXMLFile.h"

#include <itksys/SystemTools.hxx>
#include <itkMetaDataObject.h>
#include <itkIOCommon.h>

#include <iomanip>
#include "rtkMacro.h"

namespace itk
{

ThreeDCircularProjectionGeometryXMLFileReader::
ThreeDCircularProjectionGeometryXMLFileReader():
  m_Geometry(GeometryType::New() ),
  m_CurCharacterData(""),
  m_InPlaneAngle(0.),
  m_OutOfPlaneAngle(0.),
  m_GantryAngle(0.),
  m_SourceToIsocenterDistance(0.),
  m_SourceOffsetX(0.),
  m_SourceOffsetY(0.),
  m_SourceToDetectorDistance(0.),
  m_ProjectionOffsetX(0.),
  m_ProjectionOffsetY(0.)
  
{
  this->m_OutputObject = &(*m_Geometry);
}

int
ThreeDCircularProjectionGeometryXMLFileReader::
CanReadFile(const char *name)
{
  if(!itksys::SystemTools::FileExists(name) ||
     itksys::SystemTools::FileIsDirectory(name) ||
     itksys::SystemTools::FileLength(name) == 0)
    return 0;
  return 1;
}

void
ThreeDCircularProjectionGeometryXMLFileReader::
StartElement(const char * name,const char **atts)
{
  m_CurCharacterData = "";
  this->StartElement(name);
}

void
ThreeDCircularProjectionGeometryXMLFileReader::
StartElement(const char * name)
{
}

void
ThreeDCircularProjectionGeometryXMLFileReader::
EndElement(const char *name)
{
  if(itksys::SystemTools::Strucmp(name, "InPlaneAngle") == 0)
    m_InPlaneAngle = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "GantryAngle") == 0 ||
     itksys::SystemTools::Strucmp(name, "Angle") == 0) // Second one for backward compatibility
    m_GantryAngle = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "OutOfPlaneAngle") == 0)
    m_OutOfPlaneAngle = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "SourceToIsocenterDistance") == 0)
    m_SourceToIsocenterDistance = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "SourceOffsetX") == 0)
    m_SourceOffsetX = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "SourceOffsetY") == 0)
    m_SourceOffsetY = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "SourceToDetectorDistance") == 0)
    m_SourceToDetectorDistance = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "ProjectionOffsetX") == 0)
    m_ProjectionOffsetX = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "ProjectionOffsetY") == 0)
    m_ProjectionOffsetY = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "Matrix") == 0)
    {
    std::istringstream iss(this->m_CurCharacterData);
    double value = 0.;
    for(unsigned int i=0; i<m_Matrix.RowDimensions; i++)
      for(unsigned int j=0; j<m_Matrix.ColumnDimensions; j++)
        {
        iss >> value;
        m_Matrix[i][j] = value;
        }
    }

  if(itksys::SystemTools::Strucmp(name, "Projection") == 0)
    {
    this->m_OutputObject->AddProjection(m_SourceToIsocenterDistance,
                                        m_SourceToDetectorDistance,
                                        m_GantryAngle,
                                        m_ProjectionOffsetX,
                                        m_ProjectionOffsetY,
                                        m_OutOfPlaneAngle,
                                        m_InPlaneAngle,
                                        m_SourceOffsetX,
                                        m_SourceOffsetY);
    for(unsigned int i=0; i<m_Matrix.RowDimensions; i++)
      for(unsigned int j=0; j<m_Matrix.ColumnDimensions; j++)
        {
        // Tolerance can not be vcl_numeric_limits<double>::epsilon(), too strict
        // 0.001 is a random choice to catch "large" inconsistencies
        if( fabs(m_Matrix[i][j]-m_OutputObject->GetMatrices().back()[i][j]) > 0.001 )
          {
          itkGenericExceptionMacro(<< "Matrix and parameters are not consistent."
                                   << std::endl << "Read matrix from geometry file: " 
                                   << std::endl << m_Matrix
                                   << std::endl << "Computed matrix from parameters:"
                                   << std::endl << m_OutputObject->GetMatrices().back());
          }
        }
    }
}

void
ThreeDCircularProjectionGeometryXMLFileReader::
CharacterDataHandler(const char *inData, int inLength)
{
  for(int i = 0; i < inLength; i++)
    m_CurCharacterData = m_CurCharacterData + inData[i];
}

int
ThreeDCircularProjectionGeometryXMLFileWriter::
CanWriteFile(const char * name)
{
  std::ofstream output(name);

  if(output.fail() )
    return false;
  return true;
}

int
ThreeDCircularProjectionGeometryXMLFileWriter::
WriteFile()
{
  std::ofstream output(this->m_Filename.c_str() );
  const int     maxDigits = 15;

  output.precision(maxDigits);
  std::string indent("  ");

  this->WriteStartElement("?xml version=\"1.0\"?",output);
  output << std::endl;
  this->WriteStartElement("!DOCTYPE RTKGEOMETRY",output);
  output << std::endl;
  this->WriteStartElement("RTKThreeDCircularGeometry",output);
  output << std::endl;
  
  // First, we test for each of the 9 parameters per projection if it's constant
  // over all projection images except GantryAngle which is supposed to be different
  // for all projections. If 0. for OutOfPlaneAngle, InPlaneAngle, projection and source
  // offsets X and Y, it is not written (default value).
  bool bSIDGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetSourceToIsocenterDistances(),
                               "SourceToIsocenterDistance");
  bool bSDDGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetSourceToDetectorDistances(),
                               "SourceToDetectorDistance");
  bool bSourceXGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetSourceOffsetsX(),
                               "SourceOffsetX");
  bool bSourceYGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetSourceOffsetsY(),
                               "SourceOffsetY");
  bool bProjXGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetProjectionOffsetsX(),
                               "ProjectionOffsetX");
  bool bProjYGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetProjectionOffsetsY(),
                               "ProjectionOffsetY");
  bool bInPlaneGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetInPlaneAngles(),
                               "InPlaneAngle");
  bool bOutOfPlaneGlobal =
          WriteGlobalParameter(output, indent,
                               this->m_InputObject->GetOutOfPlaneAngles(),
                               "OutOfPlaneAngle");

  // Second, write per projection parameters (if corresponding parameter is not global)
  for(unsigned int i = 0; i<this->m_InputObject->GetMatrices().size(); i++)
    {
    output << indent;
    this->WriteStartElement("Projection",output);
    output << std::endl;

    // Only the GantryAngle is necessarily projection specific
    WriteLocalParameter(output, indent,
                        this->m_InputObject->GetGantryAngles()[i],
                        "GantryAngle");
    if(!bSIDGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetSourceToDetectorDistances()[i],
                          "SourceToIsocenterDistance");
    if(!bSDDGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetSourceToDetectorDistances()[i],
                          "SourceToDetectorDistance");
    if(!bSourceXGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetSourceOffsetsX()[i],
                          "SourceOffsetX");
    if(!bSourceYGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetSourceOffsetsY()[i],
                          "SourceOffsetY");
    if(!bProjXGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetProjectionOffsetsX()[i],
                          "ProjectionOffsetX");
    if(!bProjYGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetProjectionOffsetsY()[i],
                          "ProjectionOffsetY");
    if(!bInPlaneGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetInPlaneAngles()[i],
                          "InPlaneAngle");
    if(!bOutOfPlaneGlobal)
      WriteLocalParameter(output, indent,
                          this->m_InputObject->GetOutOfPlaneAngles()[i],
                          "OutOfPlaneAngle");

    //Matrix
    output << indent << indent;
    this->WriteStartElement("Matrix",output);
    output << std::endl;
    for(unsigned int j=0; j<3; j++)
      {
      output << indent << indent << indent;
      for(unsigned int k=0; k<4; k++)
        output << std::setw(maxDigits+4)
               << this->m_InputObject->GetMatrices()[i][j][k]
               << ' ';
      output.seekp(-1, std::ios_base::cur);
      output<< std::endl;
      }
    output << indent << indent;
    this->WriteEndElement("Matrix",output);
    output << std::endl;

    output << indent;
    this->WriteEndElement("Projection",output);
    output << std::endl;
    }

  this->WriteEndElement("RTKThreeDCircularGeometry",output);
  output << std::endl;

  return 0;
}

bool
ThreeDCircularProjectionGeometryXMLFileWriter::
WriteGlobalParameter(std::ofstream &output, const std::string &indent, const std::vector<double> &v, const std::string &s)
{
  // Test if all values in vector v are equal. Return false if not.
  for(size_t i=0; i<v.size(); i++)
    if(v[i] != v[0])
      return false;

  // Write value in file if not 0.
  if (0. != v[0])
    {
    std::string ss(s);
    output << indent;
    this->WriteStartElement(ss, output);
    output << v[0];
    this->WriteEndElement(ss,output);
    output << std::endl;
    }
  return true;
}

void
ThreeDCircularProjectionGeometryXMLFileWriter::
WriteLocalParameter(std::ofstream &output, const std::string &indent, const double &v, const std::string &s)
{
  std::string ss(s);
  output << indent << indent;
  this->WriteStartElement(ss, output);
  output << v;
  this->WriteEndElement(ss, output);
  output << std::endl;
}

}
#endif
