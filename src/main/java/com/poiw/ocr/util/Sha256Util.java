package com.poiw.ocr.util;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HexFormat;

public final class Sha256Util
{
    public Sha256Util()
    {
    }

    public static String calculate(byte[] data)
    {
        try
        {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hash = digest.digest(data);
            return HexFormat.of().formatHex(hash);
        } catch (NoSuchAlgorithmException e)
        {
            throw new IllegalStateException("SHA-256 algorithm not available", e);
        }
    }
}
