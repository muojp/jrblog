require 'review'
require 'tmpdir'

module ReVIEW
  class HTMLBuilder < Builder
    def image_ext
      'svg'
    end
  end
end

# module ReVIEW
#   class Builder
#     def graph_blockdiag(id, file_path, _line, tf_path)
#       system_graph(id, 'blockdiag', '-a', '-T', 'svg', '-f', '../NotoSans-Medium.ttf', '-o', file_path, tf_path)
#       file_path
#     end
#   end
# end

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
                imgpath = matches[1].sub(/^(html\/)*/, '')
                imgs.push(imgpath)
                ret.push("<img src=\"/images/%s\" alt=\"%s\" />\n" % [imgpath, matches[2]])
              else
                ret.push(line)
              end
            end
          else
        end
      end
      return ret.join, imgs
    end

    def copy_images(workdir, destdir, imgs)
      # TODO: search subdirectories recursively
      candidate_dirs = [
        File.join(workdir, 'images'),
        File.join(workdir, 'images', 'html')
      ]
      FileUtils.mkdir_p destdir
      imgs.each do |img|
        dir = candidate_dirs.detect {|dir| File.exist?(File.join(dir, img)) }
        if dir != nil
          FileUtils.cp(File.join(dir, img), File.join(destdir, img))
        end
      end
    end

    def create_blockdiag_rc_file()
      fontpath = File.join(File.expand_path(File.dirname(__FILE__)), 'NotoSansJP-Regular.otf')
      content = "[blockdiag]\nfontpath = %s" % fontpath
      File.open(File.expand_path('~/.blockdiagrc'), 'w') { |file| file.puts(content) }
    end

    def convert(content)
      create_blockdiag_rc_file()
      workdir = File.join(@config['source'], '_posts')
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
      copy_images(workdir, imgdestdir, imgs)

      # cleanup imgdir
      tmpimgdir = File.join(workdir, 'images', 'html')
      if Dir.exist?(tmpimgdir)
        #FileUtils.rm_rf(tmpimgdir)
      end
      s
    end
  end
end
