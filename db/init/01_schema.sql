DROP TYPE IF EXISTS ocr_engine;
CREATE TYPE ocr_engine AS ENUM ('NEURAL', 'TESS4J');

DROP TABLE IF EXISTS ocr_results;
DROP TABLE IF EXISTS images;

CREATE TABLE images (
                        id BIGSERIAL PRIMARY KEY,
                        hash_sha256 CHAR(64) NOT NULL UNIQUE,
                        metadata JSONB,
                        created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE ocr_results (
                             id BIGSERIAL PRIMARY KEY,
                             image_id BIGINT NOT NULL REFERENCES images(id) ON DELETE CASCADE,
                             engine ocr_engine NOT NULL,
                             recognized_text TEXT NOT NULL,
                             processing_time_ms INTEGER,
                             processed_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
                             CONSTRAINT unique_image_engine UNIQUE (image_id, engine)
);