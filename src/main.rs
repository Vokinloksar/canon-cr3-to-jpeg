use rayon::prelude::*;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::process::Command;

fn file_exists(path: &Path) -> bool {
    path.exists()
}

fn extract_thumbnail(input_file: &Path, output_file: &Path) -> bool {
    let mut in_file = match File::open(input_file) {
        Ok(file) => file,
        Err(err) => {
            eprintln!(
                "Error opening input file: {}: {}",
                input_file.display(),
                err
            );
            return false;
        }
    };

    let mut buf = Vec::new();
    if let Err(err) = in_file.read_to_end(&mut buf) {
        eprintln!("Error reading file: {}: {}", input_file.display(), err);
        return false;
    }

    let mut found = false;
    let mut n = 0;

    while n < buf.len().saturating_sub(4) && !found {
        if &buf[n..n + 4] == b"mdat" {
            if &buf[n + 12..n + 14] == b"\xFF\xD8" {
                let mut i = 0;
                while n + 12 + i < buf.len().saturating_sub(1) {
                    if &buf[n + 12 + i..n + 12 + i + 2] == b"\xFF\xD9" {
                        found = true;
                        break;
                    }
                    i += 1;
                }

                if found {
                    if let Ok(mut out_file) = File::create(output_file) {
                        if let Err(err) = out_file.write_all(&buf[n + 12..n + 12 + i + 2]) {
                            eprintln!(
                                "Error writing to output file: {}: {}",
                                output_file.display(),
                                err
                            );
                            return false;
                        }
                    } else {
                        eprintln!("Error creating output file: {}", output_file.display());
                        return false;
                    }
                }
            } else {
                break;
            }
        } else {
            n += 1;
        }
    }

    if !found {
        eprintln!("No JPEG thumbnail found in: {}", input_file.display());
        return false;
    }

    // println!("Thumbnail extracted as '{}'", output_file.display());
    true
}



fn compress_image(source_file: &Path) -> bool {
let command = format!(
        "magick \"{}\" -resize 1660x1660\\>  \"{}\" ",
        source_file.display(),
         source_file.display(),
        );
    match Command::new("sh").arg("-c").arg(command).status() {
        Ok(status) if status.success() => {
            true
        }
        _ => {
            eprintln!("Error executing command");
            false
        }
    }
}

fn copy_exif_data(source_file: &Path, destination_file: &Path) -> bool {
    // Check if lenstype is already present in the destination file
    let lenstype_present = Command::new("sh")
        .arg("-c")
        .arg(format!(
            "exiftool -s3 -lenstype \"{}\"",
            source_file.display()
        ))
        .output()
        .map(|output| !output.stdout.is_empty())
        .unwrap_or(false);

    // Copy EXIF data and add new metadata
    let command = if !lenstype_present {
        format!(
            "exiftool -overwrite_original -tagsFromFile \"{}\" -all:all \"{}\" >/dev/null \
         && exiftool -overwrite_original -exif:FocalLength=\"23\" -exif:fnumber=0 \
         -exif:MaxApertureValue=1.4 -maxAperture=1.4 \
         -lenstype=\"TTartisan EF-M 23mm f/1.4 M\" \
         -lensmodel=\"TTartisan EF-M 23mm f/1.4 M\" \"{}\" >/dev/null",
            source_file.display(),
            destination_file.display(),
            destination_file.display()
        )
    } else {
        format!(
            "exiftool -overwrite_original -tagsFromFile \"{}\" -all:all \"{}\" >/dev/null",
            source_file.display(),
            destination_file.display()
        )
    };

    match Command::new("sh").arg("-c").arg(command).status() {
        Ok(status) if status.success() => {
            // println!(
            //     "EXIF data copied from {} to {}",
            //     source_file.display(),
            //     destination_file.display()
            // );
            true
        }
        _ => {
            eprintln!("Error executing command");
            false
        }
    }
}
fn get_output_jpg_name(input_file: &Path) -> PathBuf {
    let mut output = input_file.with_extension("jpg");
    output.set_file_name(format!(
        "{}.jpg",
        input_file.file_stem().unwrap_or_default().to_string_lossy()
    ));
    output
}


fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <input_directory>", args[0]);
        std::process::exit(1);
    }

    let input_dir = Path::new(&args[1]);

    if !input_dir.is_dir() {
        eprintln!("Error: {} is not a valid directory.", args[1]);
        std::process::exit(1);
    }

    let entries: Vec<_> = fs::read_dir(input_dir)
        .unwrap()
        .filter_map(|entry| entry.ok())
        .collect();

    entries.par_iter().for_each(|entry| {
        let path = entry.path();
        if path.is_file() && path.extension().map_or(false, |ext| ext == "CR3") {
            let output_jpg = input_dir.join(get_output_jpg_name(&path));

            if file_exists(&output_jpg) {
                println!("Skip existing output file: {}", output_jpg.display());
            } else {
                if !extract_thumbnail(&path, &output_jpg) {
                    eprintln!("Failed to extract thumbnail for: {}", path.display());
                }
                if !copy_exif_data(&path, &output_jpg) {
                    eprintln!("Failed to copy EXIF data for: {}", path.display());
                }
                if !compress_image(&output_jpg) {
                    eprintln!("Failed to compress image for: {}", path.display());
                }
            }
        }
    });
}
