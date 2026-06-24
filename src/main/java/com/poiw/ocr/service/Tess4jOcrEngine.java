package com.poiw.ocr.service;

import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import com.poiw.ocr.model.StoredImageFile;
import net.sourceforge.tess4j.ITessAPI;
import net.sourceforge.tess4j.Tesseract;
import net.sourceforge.tess4j.TesseractException;
import net.sourceforge.tess4j.Word;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Set;

@Service
public class Tess4jOcrEngine implements OcrEngine
{
    private static final Logger log = LoggerFactory.getLogger(Tess4jOcrEngine.class);

    private static final String ENGINE_NAME = "TESS4J";
    private static final int BORDER_SIZE = 40;

    private static final Set<String> COMMON_WORDS = Set.of(
            "THE", "OF", "AND", "IN", "ON", "TO", "FOR", "FROM", "WITH", "WITHOUT",
            "A", "AN", "IS", "ARE", "BE", "AS", "AT", "BY", "OR", "NOT", "THIS",
            "THAT", "YOU", "YOUR", "MAN", "IMPERIUM",
            "I", "W", "Z", "NA", "DO", "OD", "DLA", "NIE", "TAK", "JEST", "ORAZ",
            "LUB", "SIE", "TEN", "TA", "TE"
    );

    private final String tessDataPath;
    private final String language;
    private final boolean debugCandidates;

    public Tess4jOcrEngine(
            @Value("${ocr.tessdata.path:tessdata}") String tessDataPath,
            @Value("${ocr.tessdata.language:eng}") String language,
            @Value("${ocr.debug-candidates:false}") boolean debugCandidates
    )
    {
        this.tessDataPath = tessDataPath;
        this.language = language;
        this.debugCandidates = debugCandidates;
    }

    @Override
    public OcrEngineType getType()
    {
        return OcrEngineType.TESS4J;
    }

    @Override
    public OcrResult recognize(StoredImageFile imageFile)
    {
        Path tessDataDirectory = resolveTessDataDirectory();
        BufferedImage originalImage = readImage(imageFile);

        List<ImageVariant> variants = createImageVariants(originalImage);
        List<OcrAttempt> attempts = new ArrayList<>();

        for (ImageVariant variant : variants)
        {
            for (int pageSegMode : variant.pageSegModes)
            {
                attempts.add(runTesseract(variant, tessDataDirectory, pageSegMode));
            }
        }

        OcrAttempt bestAttempt = chooseBestAttempt(attempts);

        if (debugCandidates)
        {
            log.info("Best OCR candidate: variant={}, psm={}, confidence={}, score={}, text={}",
                    bestAttempt.variantName,
                    pageSegModeName(bestAttempt.pageSegMode),
                    bestAttempt.confidence,
                    bestAttempt.score,
                    bestAttempt.text);
        }

        return new OcrResult(bestAttempt.text, ENGINE_NAME, bestAttempt.confidence);
    }

    private Path resolveTessDataDirectory()
    {
        Path path = Path.of(tessDataPath);

        if (!path.isAbsolute())
        {
            path = Path.of(System.getProperty("user.dir")).resolve(path);
        }

        path = path.normalize();

        if (!Files.isDirectory(path))
        {
            throw new IllegalStateException("Tessdata directory does not exist: " + path);
        }

        for (String languageCode : language.split("\\+"))
        {
            Path trainedDataFile = path.resolve(languageCode + ".traineddata");

            if (!Files.isRegularFile(trainedDataFile))
            {
                throw new IllegalStateException("Missing Tesseract language file: " + trainedDataFile);
            }
        }

        return path;
    }

    private BufferedImage readImage(StoredImageFile imageFile)
    {
        try
        {
            BufferedImage image = ImageIO.read(imageFile.path().toFile());

            if (image == null)
            {
                throw new IllegalArgumentException("Stored file is not a valid image: " + imageFile.path());
            }

            return image;
        } catch (IOException e)
        {
            throw new IllegalArgumentException("Cannot read stored image: " + imageFile.path(), e);
        }
    }

    private List<ImageVariant> createImageVariants(BufferedImage originalImage)
    {
        List<ImageVariant> variants = new ArrayList<>();

        BufferedImage fullImage = originalImage;
        BufferedImage middleImage = cropRelative(originalImage, 0.20, 0.80);
        BufferedImage bottomImage = cropRelative(originalImage, 0.50, 0.98);

        variants.add(new ImageVariant(
                "full-gray",
                addWhiteBorder(toGrayscale(fullImage, 2), BORDER_SIZE),
                new int[]{ITessAPI.TessPageSegMode.PSM_AUTO, ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT}
        ));

        variants.add(new ImageVariant(
                "full-inverted",
                addWhiteBorder(thresholdImage(fullImage, 2, true, null), BORDER_SIZE),
                new int[]{ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT}
        ));

        variants.add(new ImageVariant(
                "bottom-gray",
                addWhiteBorder(toGrayscale(bottomImage, 3), BORDER_SIZE),
                new int[]{
                        ITessAPI.TessPageSegMode.PSM_SINGLE_LINE,
                        ITessAPI.TessPageSegMode.PSM_SINGLE_BLOCK,
                        ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT
                }
        ));

        variants.add(new ImageVariant(
                "bottom-inverted",
                addWhiteBorder(thresholdImage(bottomImage, 3, true, null), BORDER_SIZE),
                new int[]{
                        ITessAPI.TessPageSegMode.PSM_SINGLE_LINE,
                        ITessAPI.TessPageSegMode.PSM_SINGLE_BLOCK,
                        ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT
                }
        ));

        variants.add(new ImageVariant(
                "middle-gray",
                addWhiteBorder(toGrayscale(middleImage, 2), BORDER_SIZE),
                new int[]{ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT}
        ));

        return variants;
    }

    private OcrAttempt runTesseract(ImageVariant variant, Path tessDataDirectory, int pageSegMode)
    {
        Tesseract tesseract = new Tesseract();

        tesseract.setDatapath(tessDataDirectory.toString());
        tesseract.setLanguage(language);
        tesseract.setOcrEngineMode(ITessAPI.TessOcrEngineMode.OEM_LSTM_ONLY);
        tesseract.setPageSegMode(pageSegMode);
        tesseract.setVariable("user_defined_dpi", "300");
        tesseract.setVariable("preserve_interword_spaces", "1");

        try
        {
            String rawText = tesseract.doOCR(variant.image);
            String cleanedText = cleanRecognizedText(rawText);
            Double confidence = calculateAverageConfidence(tesseract, variant.image);
            double score = scoreAttempt(cleanedText, confidence);

            OcrAttempt attempt = new OcrAttempt(
                    variant.name,
                    pageSegMode,
                    cleanedText,
                    confidence,
                    score
            );

            if (debugCandidates)
            {
                log.info("OCR candidate: variant={}, psm={}, confidence={}, score={}, text={}",
                        attempt.variantName,
                        pageSegModeName(attempt.pageSegMode),
                        attempt.confidence,
                        attempt.score,
                        attempt.text);
            }

            return attempt;
        } catch (TesseractException e)
        {
            throw new IllegalStateException("Tess4J OCR failed", e);
        }
    }

    private BufferedImage cropRelative(BufferedImage source, double topRatio, double bottomRatio)
    {
        int y = Math.max(0, (int) Math.round(source.getHeight() * topRatio));
        int bottom = Math.min(source.getHeight(), (int) Math.round(source.getHeight() * bottomRatio));
        int height = Math.max(1, bottom - y);

        return source.getSubimage(0, y, source.getWidth(), height);
    }

    private BufferedImage toGrayscale(BufferedImage source, int scale)
    {
        BufferedImage scaled = scaleImage(source, scale);
        BufferedImage output = new BufferedImage(scaled.getWidth(), scaled.getHeight(), BufferedImage.TYPE_BYTE_GRAY);

        Graphics2D graphics = output.createGraphics();
        graphics.drawImage(scaled, 0, 0, null);
        graphics.dispose();

        return output;
    }

    private BufferedImage thresholdImage(BufferedImage source, int scale, boolean invert, Integer fixedThreshold)
    {
        BufferedImage scaled = scaleImage(source, scale);
        int threshold = fixedThreshold != null ? fixedThreshold : calculateOtsuThreshold(scaled);

        BufferedImage output = new BufferedImage(scaled.getWidth(), scaled.getHeight(), BufferedImage.TYPE_BYTE_BINARY);

        for (int y = 0; y < scaled.getHeight(); y++)
        {
            for (int x = 0; x < scaled.getWidth(); x++)
            {
                int gray = grayAt(scaled, x, y);
                boolean bright = gray > threshold;

                int color;
                if (invert)
                {
                    color = bright ? Color.BLACK.getRGB() : Color.WHITE.getRGB();
                } else
                {
                    color = bright ? Color.WHITE.getRGB() : Color.BLACK.getRGB();
                }

                output.setRGB(x, y, color);
            }
        }

        return output;
    }

    private BufferedImage scaleImage(BufferedImage source, int scale)
    {
        int width = source.getWidth() * scale;
        int height = source.getHeight() * scale;

        BufferedImage output = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        Graphics2D graphics = output.createGraphics();

        graphics.setColor(Color.WHITE);
        graphics.fillRect(0, 0, width, height);
        graphics.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
        graphics.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        graphics.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        graphics.drawImage(source, 0, 0, width, height, null);
        graphics.dispose();

        return output;
    }

    private BufferedImage addWhiteBorder(BufferedImage source, int borderSize)
    {
        int width = source.getWidth() + borderSize * 2;
        int height = source.getHeight() + borderSize * 2;

        BufferedImage output = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        Graphics2D graphics = output.createGraphics();

        graphics.setColor(Color.WHITE);
        graphics.fillRect(0, 0, width, height);
        graphics.drawImage(source, borderSize, borderSize, null);
        graphics.dispose();

        return output;
    }

    private int calculateOtsuThreshold(BufferedImage image)
    {
        int[] histogram = new int[256];

        for (int y = 0; y < image.getHeight(); y++)
        {
            for (int x = 0; x < image.getWidth(); x++)
            {
                histogram[grayAt(image, x, y)]++;
            }
        }

        int total = image.getWidth() * image.getHeight();
        double sum = 0.0;

        for (int i = 0; i < histogram.length; i++)
        {
            sum += i * histogram[i];
        }

        double sumBackground = 0.0;
        int weightBackground = 0;
        int threshold = 127;

        double maxVariance = 0.0;

        for (int i = 0; i < histogram.length; i++)
        {
            weightBackground += histogram[i];

            if (weightBackground == 0)
            {
                continue;
            }

            int weightForeground = total - weightBackground;

            if (weightForeground == 0)
            {
                break;
            }

            sumBackground += (double) i * histogram[i];

            double meanBackground = sumBackground / weightBackground;
            double meanForeground = (sum - sumBackground) / weightForeground;
            double variance = (double) weightBackground
                    * weightForeground
                    * Math.pow(meanBackground - meanForeground, 2);

            if (variance > maxVariance)
            {
                maxVariance = variance;
                threshold = i;
            }
        }

        return threshold;
    }

    private int grayAt(BufferedImage image, int x, int y)
    {
        int rgb = image.getRGB(x, y);
        int red = (rgb >> 16) & 0xff;
        int green = (rgb >> 8) & 0xff;
        int blue = rgb & 0xff;

        return (red + green + blue) / 3;
    }

    private String cleanRecognizedText(String rawText)
    {
        if (rawText == null || rawText.isBlank())
        {
            return "";
        }

        String normalizedText = rawText
                .replace("\r\n", "\n")
                .replace("\r", "\n")
                .trim();

        String[] lines = normalizedText.split("\n");
        List<String> usefulLines = new ArrayList<>();

        for (String line : lines)
        {
            String cleanedLine = cleanLine(line);

            if (isUsefulLine(cleanedLine))
            {
                usefulLines.add(cleanedLine);
            }
        }

        if (!usefulLines.isEmpty())
        {
            return String.join("\n", usefulLines);
        }

        return "";
    }

    private String cleanLine(String line)
    {
        if (line == null)
        {
            return "";
        }

        return line
                .replaceAll("[^\\p{L}\\p{N} .,;:!?()'\"-]+", " ")
                .replaceAll("\\s+", " ")
                .trim();
    }

    private boolean isUsefulLine(String line)
    {
        if (line == null || line.isBlank())
        {
            return false;
        }

        if (countLettersOrDigits(line) < 2)
        {
            return false;
        }

        return calculateUsefulCharacterRatio(line) >= 0.65;
    }

    private double calculateUsefulCharacterRatio(String text)
    {
        if (text == null || text.isBlank())
        {
            return 0.0;
        }

        int usefulCharacters = 0;

        for (int i = 0; i < text.length(); i++)
        {
            char character = text.charAt(i);

            if (Character.isLetterOrDigit(character) || Character.isWhitespace(character))
            {
                usefulCharacters++;
            }
        }

        return (double) usefulCharacters / text.length();
    }

    private Double calculateAverageConfidence(Tesseract tesseract, BufferedImage image)
    {
        try
        {
            List<Word> words = tesseract.getWords(image, ITessAPI.TessPageIteratorLevel.RIL_WORD);

            if (words == null || words.isEmpty())
            {
                return null;
            }

            double sum = 0.0;
            int count = 0;

            for (Word word : words)
            {
                if (word.getText() != null && !word.getText().isBlank())
                {
                    sum += word.getConfidence();
                    count++;
                }
            }

            if (count == 0)
            {
                return null;
            }

            return sum / count;
        } catch (RuntimeException e)
        {
            log.warn("Cannot calculate OCR confidence", e);
            return null;
        }
    }

    private OcrAttempt chooseBestAttempt(List<OcrAttempt> attempts)
    {
        OcrAttempt bestAttempt = new OcrAttempt("", ITessAPI.TessPageSegMode.PSM_AUTO, "", null, -1_000_000.0);

        for (OcrAttempt attempt : attempts)
        {
            if (attempt.score > bestAttempt.score)
            {
                bestAttempt = attempt;
            }
        }

        return bestAttempt;
    }

    private double scoreAttempt(String text, Double confidence)
    {
        if (text == null || text.isBlank())
        {
            return -1_000_000.0;
        }

        List<String> tokens = extractTokens(text);

        int commonWords = countCommonWords(tokens);
        int longWords = countLongWords(tokens);
        int shortWords = countShortWords(tokens);
        int lettersOrDigits = countLettersOrDigits(text);

        double score = 0.0;

        if (confidence != null)
        {
            score += confidence * 4.0;

            if (confidence < 35.0)
            {
                score -= 90.0;
            }

            if (confidence >= 50.0)
            {
                score += 50.0;
            }
        }

        score += commonWords * 35.0;
        score += longWords * 10.0;
        score += lettersOrDigits * 0.25;
        score -= shortWords * 7.0;

        if (commonWords == 0 && confidence != null && confidence < 45.0)
        {
            score -= 45.0;
        }

        if (tokens.size() >= 4 && shortWords > longWords)
        {
            score -= 40.0;
        }

        return score;
    }

    private List<String> extractTokens(String text)
    {
        String[] rawTokens = text.split("\\s+");
        List<String> tokens = new ArrayList<>();

        for (String rawToken : rawTokens)
        {
            String token = rawToken.replaceAll("[^\\p{L}\\p{N}]", "");

            if (!token.isBlank())
            {
                tokens.add(token);
            }
        }

        return tokens;
    }

    private int countCommonWords(List<String> tokens)
    {
        int count = 0;

        for (String token : tokens)
        {
            String normalizedToken = token.toUpperCase(Locale.ROOT);

            if (COMMON_WORDS.contains(normalizedToken))
            {
                count++;
            }
        }

        return count;
    }

    private int countLongWords(List<String> tokens)
    {
        int count = 0;

        for (String token : tokens)
        {
            if (token.length() >= 3)
            {
                count++;
            }
        }

        return count;
    }

    private int countShortWords(List<String> tokens)
    {
        int count = 0;

        for (String token : tokens)
        {
            if (token.length() <= 2)
            {
                count++;
            }
        }

        return count;
    }

    private int countLettersOrDigits(String text)
    {
        int count = 0;

        for (int i = 0; i < text.length(); i++)
        {
            if (Character.isLetterOrDigit(text.charAt(i)))
            {
                count++;
            }
        }

        return count;
    }

    private String pageSegModeName(int pageSegMode)
    {
        if (pageSegMode == ITessAPI.TessPageSegMode.PSM_AUTO)
        {
            return "PSM_AUTO";
        }

        if (pageSegMode == ITessAPI.TessPageSegMode.PSM_SPARSE_TEXT)
        {
            return "PSM_SPARSE_TEXT";
        }

        if (pageSegMode == ITessAPI.TessPageSegMode.PSM_SINGLE_BLOCK)
        {
            return "PSM_SINGLE_BLOCK";
        }

        if (pageSegMode == ITessAPI.TessPageSegMode.PSM_SINGLE_LINE)
        {
            return "PSM_SINGLE_LINE";
        }

        return String.valueOf(pageSegMode);
    }

    private record ImageVariant(String name, BufferedImage image, int[] pageSegModes)
    {
    }

    private record OcrAttempt(String variantName, int pageSegMode, String text, Double confidence, double score)
    {
    }
}