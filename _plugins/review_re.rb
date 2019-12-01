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
              ret.push(line)
            end
          else
        end
      end
      ret.join
    end

    def convert(content)
      config = ReVIEW::Configure.values
      config['builder'] = 'html'
      config['secnolevel'] = 0 # 見出し採番しないとして
      ReVIEW::I18n.setup(config['language'])

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
      s = extract_content(s)
    end
  end
end
