#ifndef GAMEBRYOSAVEGAME_H
#define GAMEBRYOSAVEGAME_H

#include "isavegame.h"
#include "memoizedlock.h"

#include <QDateTime>
#include <QFile>
#include <QImage>
#include <QString>
#include <QStringList>

#include <stddef.h>
#include <stdexcept>

struct _SYSTEMTIME;

namespace MOBase
{
class IPluginGame;
}  // namespace MOBase

class GameGamebryo;

class GamebryoSaveGame : public MOBase::ISaveGame
{
public:
  GamebryoSaveGame(QString const& file, GameGamebryo const* game,
                   bool const lightEnabled = false);

  virtual ~GamebryoSaveGame();

public:  // ISaveGame interface
  virtual QString getFilepath() const override;
  virtual QDateTime getCreationTime() const override;
  virtual QString getName() const override;
  virtual QString getSaveGroupIdentifier() const override;
  virtual QStringList allFiles() const override;

public:
  bool hasScriptExtenderFile() const;

  // Simple getters
  virtual QString getPCName() const { return m_PCName; }
  virtual unsigned short getPCLevel() const { return m_PCLevel; }
  virtual QString getPCLocation() const { return m_PCLocation; }
  virtual unsigned long getSaveNumber() const { return m_SaveNumber; }

  QStringList const& getPlugins() const { return m_DataFields.value()->Plugins; }
  QStringList const& getLightPlugins() const
  {
    return m_DataFields.value()->LightPlugins;
  }
  QImage const& getScreenshot() const { return m_DataFields.value()->Screenshot; }

  bool isLightEnabled() const { return m_LightEnabled; }

  enum class StringType
  {
    TYPE_BZSTRING,
    TYPE_BSTRING,
    TYPE_WSTRING
  };

  enum class StringFormat
  {
    UTF8,
    LOCAL8BIT
  };

protected:
  friend class FileWrapper;

  class FileWrapper
  {
  public:
    /**
     * @brief Construct the save file information.
     *
     * @param filepath The path to the save file.
     * @params expected Expecte bytes at start of file.
     *
     **/
    FileWrapper(QString const& filepath, QString const& expected);

    /** Set this for save games that have a marker at the end of each
     * field. Specifically fallout
     **/
    void setHasFieldMarkers(bool);

    /** Set bz string mode (1 byte length, null terminated)
     **/
    void setPluginString(StringType);

    /** Set string format (utf-8, windows local 8 bit strings)
     **/
    void setPluginStringFormat(StringFormat);

    template <typename T>
    void skip(int count = 1)
    {
      if (!m_File.seek(m_File.pos() + count * sizeof(T))) {
        throw std::runtime_error("unexpected end of file");
      }
    }

    template <typename T>
    void read(T& value)
    {
      int read = m_File.read(reinterpret_cast<char*>(&value), sizeof(T));
      if (read != sizeof(T)) {
        throw std::runtime_error("unexpected end of file");
      }
      if (m_HasFieldMarkers) {
        skip<char>();
      }
    }

    template <>
    void read<QString>(QString& value);

    void seek(unsigned long pos)
    {
      if (!m_File.seek(pos)) {
        throw std::runtime_error("unexpected end of file");
      }
    }

    void read(void* buff, std::size_t length);

    /* Reads RGB image from save
     * Assumes picture dimentions come immediately before the save
     */
    QImage readImage(int scale = 0, bool alpha = false);

    /* Reads RGB image from save */
    QImage readImage(unsigned long width, unsigned long height, int scale = 0,
                     bool alpha = false);

    /* Sets the compression type. */
    void setCompressionType(uint16_t type);

    /* uncompress the begining of the compressed block */
    bool openCompressedData(int bytesToIgnore = 0);

    /* read the next compressed block */
    bool readNextChunk();

    /* frees the uncompressed block */
    void closeCompressedData();

    /* Read the save game version in the compressed block */
    uint8_t readChar(int bytesToIgnore = 0);

    uint16_t readShort(int bytesToIgnore = 0);

    uint32_t readInt(int bytesToIgnore = 0);

    uint64_t readLong(int bytesToIgnore = 0);

    float_t readFloat(int bytesToIgnore = 0);

    /* Read the plugin list */
    QStringList readPlugins(int bytesToIgnore = 0);

    /* Read the light plugin list */
    QStringList readLightPlugins(int bytesToIgnore = 0);

    void close();

  private:
    QFile m_File;
    uint64_t m_NextChunk;
    uint64_t m_UncompressedSize;
    bool m_HasFieldMarkers;
    StringType m_PluginString;
    StringFormat m_PluginStringFormat;
    QDataStream* m_Data;
    uint16_t m_CompressionType = 0;

  private:
    template <typename T>
    void readQDataStream(QDataStream& data, T& value);

    void readQDataStream(QDataStream& data, void* buff, std::size_t length);

    void skipQDataStream(QDataStream& data, std::size_t length);
  };

  void setCreationTime(_SYSTEMTIME const& time);

  GameGamebryo const* m_Game;
  bool m_LightEnabled;

  QString m_FileName;
  QString m_PCName;
  unsigned short m_PCLevel;
  QString m_PCLocation;
  unsigned long m_SaveNumber;
  QDateTime m_CreationTime;

  // Those three fields are usually much slower to fetch than
  // the other, so we do not fetch them if not needed.
  //
  // This is virtual so child class can add fields if those are
  // hard to access.
  struct DataFields
  {
    QStringList Plugins;
    QStringList LightPlugins;
    QImage Screenshot;

    // We need this constructor.
    DataFields() {}
    virtual ~DataFields() {}
  };
  MOBase::MemoizedLocked<std::unique_ptr<DataFields>> m_DataFields;

  // Fetch the field.
  virtual std::unique_ptr<DataFields> fetchDataFields() const = 0;
};

#endif  // GAMEBRYOSAVEGAME_H
