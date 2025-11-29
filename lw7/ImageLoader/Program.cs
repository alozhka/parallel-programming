using System.Diagnostics;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace ImageLoader;

file record GetDogUrlResponse(
    [property: JsonPropertyName("message")]
    string Message,
    [property: JsonPropertyName("status")] string Status
);

class Program
{
    private const uint ImageCount = 10;

    private static readonly HttpClient Client = new();

    public static async Task Main()
    {
        Directory.CreateDirectory("images_sequentially");
        Directory.CreateDirectory("images_parallel");

        Console.WriteLine("Download images sequentially");
        Stopwatch timer = Stopwatch.StartNew();
        await LoadImagesSequentially(ImageCount);
        timer.Stop();
        Console.WriteLine($"Success! Time: {timer.ElapsedMilliseconds} ms");

        Console.WriteLine("Download images in parallel");
        timer = Stopwatch.StartNew();
        await LoadImagesInParallel(ImageCount);
        timer.Stop();
        Console.WriteLine($"Success! Time: {timer.ElapsedMilliseconds} ms");
    }

    private static async Task LoadImagesInParallel(uint count)
    {
        List<Task> tasks = [];
        for (int i = 0; i < count; i++)
        {
            string filePath = Path.Combine("images_parallel", $"dog{i + 1}.jpg");
            Task task = LoadImage(filePath);
            tasks.Add(task);
        }

        await Task.WhenAll(tasks);
    }

    private static async Task<string> GetImageUrl()
    {
        string message = await Client.GetStringAsync("https://dog.ceo/api/breeds/image/random");
        GetDogUrlResponse dogUrl = JsonSerializer.Deserialize<GetDogUrlResponse>(message) ??
                                   throw new InvalidOperationException("No images found");
        return dogUrl.Message;
    }

    private static async Task LoadImagesSequentially(uint amount)
    {
        for (int i = 0; i < amount; i++)
        {
            string filePath = Path.Combine("images_sequentially", $"dog{i + 1}.jpg");
            await LoadImage(filePath);
        }
    }

    private static async Task LoadImage(string filePath)
    {
        string url = await GetImageUrl();
        Console.WriteLine($"Starting download from url: {url}");

        HttpResponseMessage response = await Client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead);
        response.EnsureSuccessStatusCode();
        await using Stream stream = await response.Content.ReadAsStreamAsync();


        await using FileStream fileStream = new(filePath, FileMode.Create, FileAccess.Write);
        await stream.CopyToAsync(fileStream);

        Console.WriteLine($"Image from {url} download successfully!");
    }
}