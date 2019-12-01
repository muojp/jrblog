require 'review'

module Jekyll
  class ReVIEWConverter < Converter
    safe true
    priority :low

    def matches(ext)
      ext =~ /^\.re$/i
    end

    def output_ext(ext)
      '.html'
    end

    def extract_content(data)
      ret = []
      imgs = []
      state = 0
      
      data.lines.each do |line|
        case state
          when 0
            if line.include? '<h1>'
              state = 1
            else
              # skip <html> / <title> / .. header tags
            end
          when 1
            if line.include? '</body>'
              state = 2
            else
              if matches = line.match(/<img src="images\/([^"]+)" alt="([^"]*)" \/>/)
                imgs.push(matches[1])
                ret.push("<img src=\"/images/%s\" alt=\"%s\" />\n" % [matches[1], matches[2]])
              else
                ret.push(line)
              end
            end
          else
        end
      end
      return ret.join, imgs
    end

    def copy_images(srcdir, destdir, imgs)
      FileUtils.mkdir_p destdir
      imgs.each do |img|
        FileUtils.cp(File.join(srcdir, img), File.join(destdir, img))
      end
    end

    def convert(content)
      workdir = File.join(@config['source'], '_posts')
      imgsrcdir = File.join(workdir, 'images')
      imgdestdir = File.join(@config['destination'], 'images')
      config = ReVIEW::Configure.values
      config['builder'] = 'html'
      config['secnolevel'] = 0 # 見出し採番しないとして
      ReVIEW::I18n.setup(config['language'])
      Dir.chdir(workdir)

      builder = ReVIEW::HTMLBuilder.new
      book = ReVIEW::Book::Base.new('.')
      book.config = config
      compiler = ReVIEW::Compiler.new(builder)
      chap = ReVIEW::Book::Chapter.new(book, '20191201', '-', nil, nil) 
      chap.content = content # 書き出したのをFile.readでもいいとは思うけど
      location = ReVIEW::Location.new(nil, nil)
      builder.bind(compiler, chap, location)
      compiler.compile(chap)
      s = builder.result
      s, imgs = extract_content(s)
      copy_images(imgsrcdir, imgdestdir, imgs)
      s
    end
  end
end
