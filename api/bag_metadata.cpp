#include "bag_dataset.h"
#include "bag_exceptions.h"
#include "bag_layer.h"
#include "bag_metadata.h"
#include "bag_metadata_export.h"
#include "bag_metadata_import.h"
#include "bag_private.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

#include <fstream>
#include <h5cpp.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif


namespace BAG
{

constexpr hsize_t kMetadataChunkSize = 1024;

Metadata::Metadata() noexcept
{
    // Can throw, but only if a new fails.  std::terminate() is fine then.
    bagInitMetadata(m_metaStruct);
}

Metadata::~Metadata() noexcept
{
    // Prevent any exceptions from escaping.
    try {
        bagFreeMetadata(m_metaStruct);
    }
    catch(...)
    {}
}

Metadata::Metadata(Dataset& dataset)
    : m_pBagDataset(dataset.shared_from_this())
{
    bagInitMetadata(m_metaStruct);

    const auto& h5file = dataset.getH5file();

    try
    {
        m_pH5DataSet = std::unique_ptr<::H5::DataSet, DeleteH5DataSet>(
            new ::H5::DataSet{h5file.openDataSet(METADATA_PATH)}, DeleteH5DataSet{});
    }
    catch(...)
    {
        throw MetadataNotFound{};
    }

    H5std_string buffer;
    const ::H5::StrType stringType{*m_pH5DataSet};
    m_pH5DataSet->read(buffer, stringType);

    this->loadFromBuffer(buffer);
}


double Metadata::columnResolution() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->columnResolution;
}

void Metadata::createH5dataSet(
    const Dataset& inDataset)
{
    m_pBagDataset = inDataset.shared_from_this();  //TODO is this always a good idea?

    const auto& h5file = inDataset.getH5file();

    const auto buffer = exportMetadataToXML(this->getStruct());
    const hsize_t xmlLength{buffer.size()};
    const hsize_t kUnlimitedSize = static_cast<hsize_t>(-1);
    const ::H5::DataSpace h5dataSpace{1, &xmlLength, &kUnlimitedSize};

    const ::H5::DSetCreatPropList h5createPropList{};
    h5createPropList.setChunk(1, &kMetadataChunkSize);

    m_pH5DataSet = std::unique_ptr<::H5::DataSet, DeleteH5DataSet>(
        new ::H5::DataSet{h5file.createDataSet(METADATA_PATH,
            ::H5::PredType::C_S1, h5dataSpace, h5createPropList)},
            DeleteH5DataSet{});

    m_pH5DataSet->extend(&xmlLength);
}

const BagMetadata& Metadata::getStruct() const noexcept
{
    return m_metaStruct;
}

size_t Metadata::getXMLlength() const noexcept
{
    return m_xmlLength;
}

const char* Metadata::horizontalCRSasWKT() const
{
    const auto* type = m_metaStruct.horizontalReferenceSystem->type;
    if (!type || std::string{type} != std::string{"WKT"})
        return nullptr;

    return m_metaStruct.horizontalReferenceSystem->definition;
}

double Metadata::llCornerX() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->llCornerX;
}

double Metadata::llCornerY() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->llCornerY;
}

void Metadata::loadFromFile(const std::string& fileName)
{
    const BagError err = bagImportMetadataFromXmlFile(fileName.c_str(),
        m_metaStruct, false);
    if (err)
        throw err;

    std::ifstream ifs{fileName, std::ios_base::in|std::ios_base::binary};
    ifs.seekg(0, std::ios_base::end);
    m_xmlLength = ifs.tellg();
}

void Metadata::loadFromBuffer(const std::string& xmlBuffer)
{
    const BagError err = bagImportMetadataFromXmlBuffer(xmlBuffer.c_str(),
        static_cast<int>(xmlBuffer.size()), m_metaStruct, false);
    if (err)
        throw err;

    m_xmlLength = xmlBuffer.size();
}

double Metadata::rowResolution() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->rowResolution;
}

double Metadata::urCornerX() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->urCornerX;
}

double Metadata::urCornerY() const noexcept
{
    return m_metaStruct.spatialRepresentationInfo->urCornerY;
}

void Metadata::DeleteH5DataSet::operator()(::H5::DataSet* ptr) noexcept
{
    delete ptr;
}

void Metadata::write() const
{
    const auto buffer = exportMetadataToXML(this->getStruct());

    const hsize_t bufferLen = buffer.size();
    const hsize_t kMaxSize = static_cast<hsize_t>(-1);
    const ::H5::DataSpace h5dataSpace{1, &bufferLen, &kMaxSize};

    m_pH5DataSet->write(buffer, ::H5::PredType::C_S1, h5dataSpace);
}

}   //namespace BAG


