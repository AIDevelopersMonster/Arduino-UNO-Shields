using System;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;

// Tests a uniquely owned ordinary file, never the raw device or existing files.
public sealed class ArduinoUsbSdMeasurement
{
    public long Bytes;
    public long ElapsedMs;
    public string Sha256;
}

public static class ArduinoUsbSdProbe
{
    public const string Version = "0.1";
    private const int ChunkSize = 65536;

    private static void Deadline(Stopwatch watch, int seconds)
    {
        if (watch.Elapsed.TotalSeconds > seconds)
            throw new IOException("File phase exceeded " + seconds + " seconds.");
    }

    private static string Hex(byte[] bytes)
    {
        return BitConverter.ToString(bytes).Replace("-", "");
    }

    public static ArduinoUsbSdMeasurement Write(string path, long bytes, int seed, int seconds)
    {
        var watch = Stopwatch.StartNew();
        var random = new Random(seed);
        var buffer = new byte[ChunkSize];
        string digest;
        using (var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256))
        {
            using (var file = new FileStream(path, FileMode.CreateNew, FileAccess.Write,
                                            FileShare.None, ChunkSize, FileOptions.WriteThrough))
            {
                for (long offset = 0; offset < bytes; offset += ChunkSize)
                {
                    Deadline(watch, seconds);
                    int count = (int)Math.Min(ChunkSize, bytes - offset);
                    random.NextBytes(buffer);
                    file.Write(buffer, 0, count);
                    hash.AppendData(buffer, 0, count);
                }
                file.Flush(true);
                if (file.Length != bytes) throw new IOException("Written length differs.");
            }
            digest = Hex(hash.GetHashAndReset());
        }
        Deadline(watch, seconds);
        return new ArduinoUsbSdMeasurement { Bytes = bytes, ElapsedMs = watch.ElapsedMilliseconds, Sha256 = digest };
    }

    public static ArduinoUsbSdMeasurement Verify(string path, long bytes, int seed, int seconds)
    {
        var watch = Stopwatch.StartNew();
        var random = new Random(seed);
        var expected = new byte[ChunkSize];
        var actual = new byte[ChunkSize];
        string digest;
        using (var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256))
        {
            using (var file = new FileStream(path, FileMode.Open, FileAccess.Read,
                                            FileShare.None, ChunkSize, FileOptions.SequentialScan))
            {
                if (file.Length != bytes) throw new IOException("Readback length differs: " + file.Length);
                for (long offset = 0; offset < bytes; offset += ChunkSize)
                {
                    Deadline(watch, seconds);
                    int count = (int)Math.Min(ChunkSize, bytes - offset);
                    random.NextBytes(expected);
                    int received = 0;
                    while (received < count)
                    {
                        Deadline(watch, seconds);
                        int read = file.Read(actual, received, count - received);
                        if (read == 0) throw new IOException("Unexpected EOF at byte " + (offset + received));
                        received += read;
                    }
                    for (int i = 0; i < count; ++i)
                        if (actual[i] != expected[i])
                            throw new IOException("Data mismatch at byte " + (offset + i));
                    hash.AppendData(actual, 0, count);
                }
                if (file.ReadByte() != -1) throw new IOException("Extra trailing data.");
            }
            digest = Hex(hash.GetHashAndReset());
        }
        Deadline(watch, seconds);
        return new ArduinoUsbSdMeasurement { Bytes = bytes, ElapsedMs = watch.ElapsedMilliseconds, Sha256 = digest };
    }
}
