namespace AsyncWriter;

internal static class AsyncWriter
{
    public static async Task Write(string filename, IReadOnlySet<char> symbols)
    {
        string content = await File.ReadAllTextAsync(filename);
        string processedContent = new(content.Where(ch => !symbols.Contains(ch)).ToArray());
        await File.WriteAllTextAsync(filename, processedContent);
    }
}

class Program
{
    public static async Task Main(string[] args)
    {
        try
        {
            string filename = args[0];
            Console.Write("Enter symbols to delete(no separation): ");
            HashSet<char> symbols = Console.ReadLine()!.ToHashSet();

            await AsyncWriter.Write(filename, symbols);
        }
        catch (Exception e)
        {
            Console.WriteLine($"Error occured: {e.Message}");
        }
    }
}